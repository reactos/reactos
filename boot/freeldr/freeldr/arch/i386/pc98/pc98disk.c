/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Drive access routines for NEC PC-98 series
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#include <hwide.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(DISK);

/* GLOBALS ********************************************************************/

/* Maximum number of disks, indexed from 0x00 to 0xFF */
#define MAX_DRIVES 0x100

/* Cache of all possible PC disk drives */
static PC98_DISK_DRIVE Pc98DiskDrive[MAX_DRIVES];

/* DISK IO ERROR SUPPORT ******************************************************/

static LONG lReportError = 0; /* >= 0: display errors; < 0: hide errors */

LONG
DiskReportError(
    _In_ BOOLEAN bShowError)
{
    /* Set the reference count */
    if (bShowError)
        ++lReportError;
    else
        --lReportError;
    return lReportError;
}

static
PCSTR
DiskGetErrorCodeString(
    _In_ ULONG ErrorCode)
{
    switch (ErrorCode & 0xF0)
    {
        case 0x00: return "No error";
        case 0x10: return "Drive write protection error";
        case 0x20: return "DMA access across 64 kB boundary";
        case 0x30: return "End of cylinder";
        case 0x40: return "The drive name is invalid or the device have low health";
        case 0x50: return "Time out, data not written";
        case 0x60: return "Time out, drive not ready";
        case 0x70:
            if (ErrorCode == 0x78)
                return "Illegal disk address";
            else
                return "Drive write protection error";
        case 0x80: return "Undefined error";
        case 0x90: return "Time out error";
        case 0xA0: return "CRC error in the ID section";
        case 0xB0: return "CRC error in the DATA section";
        case 0xC0:
            if (ErrorCode == 0xC8)
                return "Seek failure";
            else
                return "No data (Sector not found)";
        case 0xD0: return "Bad cylinder";
        case 0xE0: return "No ID address mark was found";
        case 0xF0: return "No DATA address mark was found";

        default: return "Unknown error code";
    }
}

static
VOID
DiskError(
    _In_ PCSTR ErrorString,
    _In_ ULONG ErrorCode)
{
    CHAR ErrorCodeString[200];

    if (lReportError < 0)
        return;

    RtlStringCbPrintfA(ErrorCodeString,
                       sizeof(ErrorCodeString),
                       "%s\n\nError Code: 0x%lx\nError: %s",
                       ErrorString,
                       ErrorCode,
                       DiskGetErrorCodeString(ErrorCode));

    ERR("%s\n", ErrorCodeString);

    UiMessageBox(ErrorCodeString);
}

/* FUNCTIONS ******************************************************************/

static
BOOLEAN
DiskResetController(
    _In_ PPC98_DISK_DRIVE DiskDrive)
{
    REGS Regs;

    if (DiskDrive->Type == DRIVE_TYPE_FDD)
    {
        /* Int 1Bh AH=07h
         * DISK BIOS - Recalibrate
         *
         * Call with:
         * AL - drive number
         *
         * Return:
         * CF - set on error, clear if successful
         * AH - status
         */
        Regs.b.ah = 0x07;
    }
    else if (DiskDrive->Type == DRIVE_TYPE_HDD)
    {
        /* Int 1Bh AH=03h
         * DISK BIOS - Initialize
         *
         * Call with:
         * AL - drive number
         *
         * Return:
         * CF - set on error, clear if successful
         * AH - status
         */
        Regs.b.ah = 0x03;
    }
    else
    {
        return FALSE;
    }

    WARN("DiskResetController(0x%x) DISK OPERATION FAILED -- RESETTING CONTROLLER\n",
         DiskDrive->DaUa);

    Regs.b.al = DiskDrive->DaUa;
    Int386(0x1B, &Regs, &Regs);
    return INT386_SUCCESS(Regs);
}

PPC98_DISK_DRIVE
Pc98DiskDriveNumberToDrive(
    _In_ UCHAR DriveNumber)
{
    PPC98_DISK_DRIVE DiskDrive;

    ASSERT(DriveNumber < RTL_NUMBER_OF(Pc98DiskDrive));

    /* Retrieve a slot */
    DiskDrive = &Pc98DiskDrive[DriveNumber];

    /* The pre-initialization of the BIOS disks was already done in Pc98InitializeBootDevices() */
    if (DiskDrive->Flags & DRIVE_FLAGS_INITIALIZED)
        return DiskDrive;
    else
        return NULL;
}

CONFIGURATION_TYPE
DiskGetConfigType(
    _In_ UCHAR DriveNumber)
{
    PPC98_DISK_DRIVE DiskDrive;

    DiskDrive = Pc98DiskDriveNumberToDrive(DriveNumber);
    if (!DiskDrive)
        return -1; // MaximumType;

    if (DiskDrive->Type == DRIVE_TYPE_CDROM)
        return CdromController;
    else if (DiskDrive->Type == DRIVE_TYPE_FDD)
        return FloppyDiskPeripheral;
    else
        return DiskPeripheral;
}

static
UCHAR
BytesPerSectorToSectorLengthCode(
    _In_ ULONG BytesPerSector)
{
    switch (BytesPerSector)
    {
        case 128:
            return 0;
        case 256:
            return 1;
        case 512:
            return 2;
        case 1024:
            return 3;
        case 2048:
            return 4;
        default:
            return 0;
    }
}

static
BOOLEAN
Pc98DiskReadLogicalSectorsLBA(
    _In_ PPC98_DISK_DRIVE DiskDrive,
    _In_ ULONG64 SectorNumber,
    _In_ ULONG SectorCount,
    _Out_writes_bytes_all_(SectorCount * DiskDrive->Geometry.BytesPerSector) PVOID Buffer)
{
    REGS RegsIn, RegsOut;
    ULONG RetryCount;

    /* Int 1Bh AH=06h
     * DISK BIOS - Read data
     *
     * Call with:
     * AL - drive number
     * BX - bytes to read
     * CX - cylinder number
     * DH - head number
     * DL - sector number
     * ES:BP -> buffer to read data into
     *
     * Return:
     * CF - set on error, clear if successful
     * AH - status
     */
    RegsIn.b.al = DiskDrive->DaUa;
    RegsIn.b.ah = 0x06;
    RegsIn.w.bx = DiskDrive->Geometry.BytesPerSector * SectorCount;
    RegsIn.w.cx = SectorNumber & 0xFFFF;
    RegsIn.w.dx = (SectorNumber >> 16) & 0xFFFF;
    RegsIn.w.es = (USHORT)(((ULONG_PTR)Buffer) >> 4);
    RegsIn.w.bp = ((ULONG_PTR)Buffer) & 0x0F;

    /* Retry 3 times */
    for (RetryCount = 0; RetryCount < 3; ++RetryCount)
    {
        Int386(0x1B, &RegsIn, &RegsOut);

        /* If it worked, or if it was a corrected ECC error
         * and the data is still good, return success */
        if (INT386_SUCCESS(RegsOut) || (RegsOut.b.ah == 0x08))
            return TRUE;

        /* It failed, do the next retry */
        DiskResetController(DiskDrive);
    }

    /* If we get here then the read failed */
    DiskError("Disk Read Failed in LBA mode", RegsOut.b.ah);
    ERR("Disk Read Failed in LBA mode: %x (%s) "
        "(DriveNumber: 0x%x SectorNumber: %I64u SectorCount: %u)\n",
        RegsOut.b.ah, DiskGetErrorCodeString(RegsOut.b.ah),
        DiskDrive->DaUa, SectorNumber, SectorCount);

    return FALSE;
}

static
BOOLEAN
Pc98DiskReadLogicalSectorsCHS(
    _In_ PPC98_DISK_DRIVE DiskDrive,
    _In_ ULONG64 SectorNumber,
    _In_ ULONG SectorCount,
    _Out_writes_bytes_all_(SectorCount * DiskDrive->Geometry.BytesPerSector) PVOID Buffer)
{
    UCHAR PhysicalSector;
    UCHAR PhysicalHead;
    ULONG PhysicalTrack;
    GEOMETRY DriveGeometry;
    ULONG NumberOfSectorsToRead;
    REGS RegsIn, RegsOut;
    ULONG RetryCount;

    DriveGeometry = DiskDrive->Geometry;

    while (SectorCount > 0)
    {
        /*
         * Calculate the physical disk offsets.
         * Note: DriveGeometry.SectorsPerTrack < 64
         */
        PhysicalSector = (UCHAR)(SectorNumber % DriveGeometry.SectorsPerTrack);
        PhysicalHead = (UCHAR)((SectorNumber / DriveGeometry.SectorsPerTrack) % DriveGeometry.Heads);
        PhysicalTrack = (ULONG)((SectorNumber / DriveGeometry.SectorsPerTrack) / DriveGeometry.Heads);

        /* Floppy sectors value always start at 1 */
        if (DiskDrive->Type == DRIVE_TYPE_FDD)
            ++PhysicalSector;

        /* Calculate how many sectors we need to read this round */
        if (PhysicalSector > 1)
        {
            NumberOfSectorsToRead = min(SectorCount,
                                        (DriveGeometry.SectorsPerTrack - (PhysicalSector - 1)));
        }
        else
        {
            NumberOfSectorsToRead = min(SectorCount, DriveGeometry.SectorsPerTrack);
        }

        /* Make sure the read is within the geometry boundaries */
        if ((PhysicalHead >= DriveGeometry.Heads) ||
            (PhysicalTrack >= DriveGeometry.Cylinders) ||
            ((NumberOfSectorsToRead + PhysicalSector) > (DriveGeometry.SectorsPerTrack + 1)) ||
            (PhysicalSector > DriveGeometry.SectorsPerTrack))
        {
            DiskError("Disk read exceeds drive geometry limits.", 0);
            return FALSE;
        }

        if (DiskDrive->Type == DRIVE_TYPE_FDD)
        {
            /* Int 1Bh AH=x6h
             * DISK BIOS - Read data
             *
             * Call with:
             * AL - drive number
             * BX - bytes to read
             * CH - sector length code
             * CL - cylinder number
             * DH - head number
             * DL - sector number
             * ES:BP -> buffer to read data into
             *
             * Return:
             * CF - set on error, clear if successful
             * AH - status
             */
            RegsIn.b.al = DiskDrive->DaUa;
            RegsIn.b.ah = 0x56;    /* With SEEK, and use double-density format (MFM) */
            RegsIn.w.bx = DriveGeometry.BytesPerSector * (UCHAR)NumberOfSectorsToRead;
            RegsIn.b.cl = PhysicalTrack & 0xFFFF;
            RegsIn.b.ch = BytesPerSectorToSectorLengthCode(DriveGeometry.BytesPerSector);
            RegsIn.b.dl = PhysicalSector;
            RegsIn.b.dh = PhysicalHead;
        }
        else
        {
            /* Int 1Bh AH=06h
             * DISK BIOS - Read data
             *
             * Call with:
             * AL - drive number
             * BX - bytes to read
             * CX - cylinder number
             * DH - head number
             * DL - sector number
             * ES:BP -> buffer to read data into
             *
             * Return:
             * CF - set on error, clear if successful
             * AH - status
             */
            RegsIn.b.al = DiskDrive->DaUa;
            RegsIn.b.ah = 0x06;
            RegsIn.w.bx = DriveGeometry.BytesPerSector * (UCHAR)NumberOfSectorsToRead;
            RegsIn.w.cx = PhysicalTrack & 0xFFFF;
            RegsIn.b.dl = PhysicalSector;
            RegsIn.b.dh = PhysicalHead;
        }
        RegsIn.w.es = (USHORT)(((ULONG_PTR)Buffer) >> 4);
        RegsIn.w.bp = ((ULONG_PTR)Buffer) & 0x0F;

        /* Perform the read. Retry 3 times. */
        for (RetryCount = 0; RetryCount < 3; ++RetryCount)
        {
            Int386(0x1B, &RegsIn, &RegsOut);

            /* If it worked, or if it was a corrected ECC error
             * and the data is still good, return success */
            if (INT386_SUCCESS(RegsOut) || (RegsOut.b.ah == 0x08))
                break;

            /* It failed, do the next retry */
            DiskResetController(DiskDrive);
        }

        /* If we retried 3 times then fail */
        if (RetryCount >= 3)
        {
            DiskError("Disk Read Failed in CHS mode, after retrying 3 times", RegsOut.b.ah);
            ERR("Disk Read Failed in CHS mode, after retrying 3 times: %x (%s) "
                "(DriveNumber: 0x%x SectorNumber: %I64u SectorCount: %u)\n",
                RegsOut.b.ah, DiskGetErrorCodeString(RegsOut.b.ah),
                DiskDrive->DaUa, SectorNumber, SectorCount);
            return FALSE;
        }

        Buffer = (PVOID)((ULONG_PTR)Buffer + (NumberOfSectorsToRead * DriveGeometry.BytesPerSector));
        SectorCount -= NumberOfSectorsToRead;
        SectorNumber += NumberOfSectorsToRead;
    }

    return TRUE;
}

static
BOOLEAN
InitScsiDrive(
    _Out_ PPC98_DISK_DRIVE DiskDrive,
    _In_ UCHAR DaUa)
{
    REGS RegsIn, RegsOut;
    UCHAR UnitAddress = DaUa & 0x0F;
    USHORT DiskEquipment = *(PUCHAR)MEM_DISK_EQUIPS;
    ULONG ScsiParameters = *(PULONG)(MEM_SCSI_TABLE + UnitAddress * sizeof(ULONG));

    /* Hard drives */
    if (DiskEquipment & (1 << UnitAddress))
    {
        /* Int 1Bh AH=84h
         * DISK BIOS - Sense
         *
         * Call with:
         * AL - drive number
         *
         * Return:
         * BX - bytes per sector
         * CX - cylinders number
         * DH - heads number
         * DL - sectors number
         * CF - set on error, clear if successful
         * AH - status
         */
        RegsIn.b.al = DaUa;
        RegsIn.b.ah = 0x84;
        Int386(0x1B, &RegsIn, &RegsOut);
        if (!INT386_SUCCESS(RegsOut) || RegsOut.w.cx == 0)
            return FALSE;

        DiskDrive->Geometry.Cylinders = RegsOut.w.cx;
        DiskDrive->Geometry.Heads = RegsOut.b.dh;
        DiskDrive->Geometry.SectorsPerTrack = RegsOut.b.dl;
        DiskDrive->Geometry.BytesPerSector = RegsOut.w.bx;
    }
    /* Other devices */
    else if (ScsiParameters != 0)
    {
        UCHAR DeviceType;

        DeviceType = ScsiParameters & 0x1F;
        switch (DeviceType)
        {
            case 0x05:
                /* CD-ROM */
                DiskDrive->Geometry.Cylinders = 0xFFFF;
                DiskDrive->Geometry.Heads = 0xFFFF;
                DiskDrive->Geometry.SectorsPerTrack = 0xFFFF;
                DiskDrive->Geometry.BytesPerSector = 2048;

                DiskDrive->Type = DRIVE_TYPE_CDROM;
                DiskDrive->Flags = DRIVE_FLAGS_LBA | DRIVE_FLAGS_REMOVABLE;
                break;

            case 0x07:
                /* Magneto-optical drive */
                DiskDrive->Geometry.Cylinders = 0xFFFF;
                DiskDrive->Geometry.Heads = 8;
                DiskDrive->Geometry.SectorsPerTrack = 32;
                DiskDrive->Geometry.BytesPerSector = 512;

                DiskDrive->Type = DRIVE_TYPE_CDROM;
                DiskDrive->Flags = DRIVE_FLAGS_LBA | DRIVE_FLAGS_REMOVABLE;
                break;

            default:
                return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    DiskDrive->Flags |= DRIVE_FLAGS_INITIALIZED;
    DiskDrive->DaUa = DaUa;

    DiskDrive->Geometry.Sectors = (ULONGLONG)DiskDrive->Geometry.Cylinders *
                                             DiskDrive->Geometry.Heads *
                                             DiskDrive->Geometry.SectorsPerTrack;

    TRACE("InitScsiDrive(0x%x) returned:\n"
          "Cylinders  : 0x%x\n"
          "Heads      : 0x%x\n"
          "Sects/Track: 0x%x\n"
          "Total Sects: 0x%llx\n"
          "Bytes/Sect : 0x%x\n",
          DaUa,
          DiskDrive->Geometry.Cylinders,
          DiskDrive->Geometry.Heads,
          DiskDrive->Geometry.SectorsPerTrack,
          DiskDrive->Geometry.Sectors,
          DiskDrive->Geometry.BytesPerSector);

    return TRUE;
}

static
BOOLEAN
InitFloppyDrive(
    _Out_ PPC98_DISK_DRIVE DiskDrive,
    _In_ UCHAR DaUa)
{
    REGS RegsIn, RegsOut;
    ULONG BytesPerSector;
    UCHAR DeviceAddress;

    /* Int 1Bh AH=4Ah
     * DISK BIOS - Read ID
     *
     * Call with:
     * AL - drive number
     *
     * Return:
     * CH - sector size
     * CL - cylinder
     * DH - head
     * DL - sector
     * CF - set on error, clear if successful
     * AH - status
     */
    RegsIn.b.ah = 0x4A;
    RegsIn.b.al = DaUa;
    Int386(0x1B, &RegsIn, &RegsOut);
    if (!INT386_SUCCESS(RegsOut))
        return FALSE;

    /* There is no way to obtain floppy disk geometry in BIOS */
    DeviceAddress = DaUa & 0xF0;
    BytesPerSector = 128 << RegsOut.b.ch;
    switch (BytesPerSector)
    {
        case 256:
            if (DeviceAddress == 0x50)
            {
                /* 320 kB 2DD */
                DiskDrive->Geometry.Cylinders = 80;
                DiskDrive->Geometry.Heads = 2;
                DiskDrive->Geometry.SectorsPerTrack = 16;
            }
            else
            {
                /* 1 MB 2HD */
                DiskDrive->Geometry.Cylinders = 77;
                DiskDrive->Geometry.Heads = 2;
                DiskDrive->Geometry.SectorsPerTrack = 26;
            }
            break;

        case 512:
            if (DeviceAddress == 0x30 || DeviceAddress == 0xB0)
            {
                /* 1.44 MB 2HD */
                DiskDrive->Geometry.Cylinders = 80;
                DiskDrive->Geometry.Heads = 2;
                DiskDrive->Geometry.SectorsPerTrack = 18;
            }
            else if (DeviceAddress == 0x70 || DeviceAddress == 0xF0)
            {
                /* 720/640 kB 2DD */
                DiskDrive->Geometry.Cylinders = 80;
                DiskDrive->Geometry.Heads = 2;
                DiskDrive->Geometry.SectorsPerTrack = 8;
            }
            else
            {
                /* 1.2 MB 2HC */
                DiskDrive->Geometry.Cylinders = 80;
                DiskDrive->Geometry.Heads = 2;
                DiskDrive->Geometry.SectorsPerTrack = 15;
            }
            break;

        case 1024:
            /* 1.25 MB 2HD */
            DiskDrive->Geometry.Cylinders = 77;
            DiskDrive->Geometry.Heads = 2;
            DiskDrive->Geometry.SectorsPerTrack = 8;
            break;

        default:
            return FALSE;
    }

    DiskDrive->Geometry.BytesPerSector = BytesPerSector;
    DiskDrive->Geometry.Sectors = (ULONGLONG)DiskDrive->Geometry.Cylinders *
                                             DiskDrive->Geometry.Heads *
                                             DiskDrive->Geometry.SectorsPerTrack;

    DiskDrive->DaUa = DaUa;
    DiskDrive->Type = DRIVE_TYPE_FDD;
    DiskDrive->Flags = DRIVE_FLAGS_REMOVABLE | DRIVE_FLAGS_INITIALIZED;

    TRACE("InitFloppyDrive(0x%x) returned:\n"
          "Cylinders  : 0x%x\n"
          "Heads      : 0x%x\n"
          "Sects/Track: 0x%x\n"
          "Total Sects: 0x%llx\n"
          "Bytes/Sect : 0x%x\n",
          DaUa,
          DiskDrive->Geometry.Cylinders,
          DiskDrive->Geometry.Heads,
          DiskDrive->Geometry.SectorsPerTrack,
          DiskDrive->Geometry.Sectors,
          DiskDrive->Geometry.BytesPerSector);

    return TRUE;
}

static
BOOLEAN
InitIdeDrive(
    _Out_ PPC98_DISK_DRIVE DiskDrive,
    _In_ UCHAR AtaUnitNumber)
{
    PDEVICE_UNIT DeviceUnit = AtaGetDevice(AtaUnitNumber);

    if (!DeviceUnit)
        return FALSE;

    DiskDrive->Geometry.Cylinders = DeviceUnit->Cylinders;
    DiskDrive->Geometry.Heads = DeviceUnit->Heads;
    DiskDrive->Geometry.SectorsPerTrack = DeviceUnit->SectorsPerTrack;
    DiskDrive->Geometry.BytesPerSector = DeviceUnit->SectorSize;
    DiskDrive->Geometry.Sectors = DeviceUnit->TotalSectors;

    DiskDrive->DaUa = 0xFF; // Invalid
    DiskDrive->AtaUnitNumber = AtaUnitNumber;
    DiskDrive->Flags = DRIVE_FLAGS_IDE | DRIVE_FLAGS_INITIALIZED;

    if (DeviceUnit->Flags & ATA_DEVICE_LBA)
        DiskDrive->Flags |= DRIVE_FLAGS_LBA;

    if (DeviceUnit->Flags & ATA_DEVICE_ATAPI)
        DiskDrive->Type = DRIVE_TYPE_CDROM;
    else
        DiskDrive->Type = DRIVE_TYPE_HDD;

    TRACE("InitIdeDrive(0x%x) returned:\n"
          "Cylinders  : 0x%x\n"
          "Heads      : 0x%x\n"
          "Sects/Track: 0x%x\n"
          "Total Sects: 0x%llx\n"
          "Bytes/Sect : 0x%x\n",
          AtaUnitNumber,
          DiskDrive->Geometry.Cylinders,
          DiskDrive->Geometry.Heads,
          DiskDrive->Geometry.SectorsPerTrack,
          DiskDrive->Geometry.Sectors,
          DiskDrive->Geometry.BytesPerSector);
    return TRUE;
}

BOOLEAN
Pc98InitializeBootDevices(VOID)
{
    USHORT DiskEquipment = *(PUSHORT)MEM_DISK_EQUIP & ~(*(PUCHAR)MEM_RDISK_EQUIP);
    PPC98_DISK_DRIVE DiskDrive;
    UCHAR BiosFloppyDriveNumber, BiosHardDriveDriveNumber, IdeDetectedCount;
    ULONG i;

    TRACE("Pc98InitializeBootDevices()\n");

    RtlZeroMemory(&Pc98DiskDrive, sizeof(Pc98DiskDrive));

    /*
     * We emulate the standard PC BIOS drive numbers here. Map DA/UA to a drive number, i.e.
     * 0x90 -> 0x30
     * 0x80 -> 0x80
     * 0xA0 -> 0x81, etc.
     */
    BiosFloppyDriveNumber = 0x30;
    BiosHardDriveDriveNumber = 0x80;

    /* Map floppies */
    for (i = 0; i < 4; i++)
    {
        DiskDrive = &Pc98DiskDrive[BiosFloppyDriveNumber];
        if (FIRSTBYTE(DiskEquipment) & (1 << i))
        {
            if (InitFloppyDrive(DiskDrive, 0x30 + i) || InitFloppyDrive(DiskDrive, 0xB0 + i) ||
                InitFloppyDrive(DiskDrive, 0x90 + i) || InitFloppyDrive(DiskDrive, 0x10 + i))
                ++BiosFloppyDriveNumber;
        }
    }
    for (i = 0; i < 4; i++)
    {
        DiskDrive = &Pc98DiskDrive[BiosFloppyDriveNumber];
        if (FIRSTBYTE(DiskEquipment) & (16 << i))
        {
            if (InitFloppyDrive(DiskDrive, 0x50 + i))
                ++BiosFloppyDriveNumber;
        }
    }
    for (i = 0; i < 4; i++)
    {
        DiskDrive = &Pc98DiskDrive[BiosFloppyDriveNumber];
        if (SECONDBYTE(DiskEquipment) & (16 << i))
        {
            if (InitFloppyDrive(DiskDrive, 0x70 + i) || InitFloppyDrive(DiskDrive, 0xF0 + i))
                ++BiosFloppyDriveNumber;
        }
    }

    /*
     * Map IDE drives. We provide our own IDE boot support because of IDE BIOS
     * that cannot boot from a CD-ROM and has LBA limitations.
     */
    AtaInit(&IdeDetectedCount);
    for (i = 0; i <= IdeDetectedCount; i++)
    {
        DiskDrive = &Pc98DiskDrive[BiosHardDriveDriveNumber];
        if (InitIdeDrive(DiskDrive, i))
            ++BiosHardDriveDriveNumber;
    }

    /* Map SCSI drives */
    for (i = 0; i < 7; i++)
    {
        DiskDrive = &Pc98DiskDrive[BiosHardDriveDriveNumber];
        if (InitScsiDrive(DiskDrive, 0xA0 + i) || InitScsiDrive(DiskDrive, 0x20 + i))
            ++BiosHardDriveDriveNumber;
    }

    // Ugly HACK: Force ISO boot
    // FIXME: Fill ARC disk blocks completely
    // to allow usage of CD-ROM root path (See floppy_pc98.ini).
    for (i = 0x80; i < RTL_NUMBER_OF(Pc98DiskDrive); i++)
    {
        DiskDrive = &Pc98DiskDrive[i];

        if ((DiskDrive->Flags & DRIVE_FLAGS_INITIALIZED) &&
            (DiskDrive->Flags & DRIVE_FLAGS_IDE) &&
            (DiskDrive->Type == DRIVE_TYPE_CDROM))
        {
            FrldrBootDrive = i;
            FrldrBootPartition = 0xFF;
            break;
        }
    }

    /* Call PC version */
    return PcInitializeBootDevices();
}

BOOLEAN
Pc98DiskReadLogicalSectors(
    _In_ UCHAR DriveNumber,
    _In_ ULONGLONG SectorNumber,
    _In_ ULONG SectorCount,
    _Out_ PVOID Buffer)
{
    PPC98_DISK_DRIVE DiskDrive;

    TRACE("Pc98DiskReadLogicalSectors() "
          "DriveNumber: 0x%x SectorNumber: %I64u SectorCount: %u Buffer: 0x%x\n",
          DriveNumber, SectorNumber, SectorCount, Buffer);

    /* 16-bit BIOS addressing limitation */
    ASSERT(((ULONG_PTR)Buffer) <= 0xFFFFF);

    DiskDrive = Pc98DiskDriveNumberToDrive(DriveNumber);
    if (!DiskDrive)
        return FALSE;

    if (DiskDrive->Flags & DRIVE_FLAGS_IDE)
    {
        return AtaReadLogicalSectors(AtaGetDevice(DiskDrive->AtaUnitNumber),
                                     SectorNumber,
                                     SectorCount,
                                     Buffer);
    }
    else if (DiskDrive->Flags & DRIVE_FLAGS_LBA)
    {
        /* LBA is easy, nothing to calculate. Just do the read. */
        TRACE("--> Using LBA\n");
        return Pc98DiskReadLogicalSectorsLBA(DiskDrive, SectorNumber, SectorCount, Buffer);
    }
    else
    {
        /* LBA is not supported, default to CHS */
        TRACE("--> Using CHS\n");
        return Pc98DiskReadLogicalSectorsCHS(DiskDrive, SectorNumber, SectorCount, Buffer);
    }
}

BOOLEAN
Pc98DiskGetDriveGeometry(
    _In_ UCHAR DriveNumber,
    _Out_ PGEOMETRY Geometry)
{
    PPC98_DISK_DRIVE DiskDrive;

    TRACE("Pc98DiskGetDriveGeometry(0x%x)\n", DriveNumber);

    DiskDrive = Pc98DiskDriveNumberToDrive(DriveNumber);
    if (!DiskDrive)
        return FALSE;

    *Geometry = DiskDrive->Geometry;

    return TRUE;
}

ULONG
Pc98DiskGetCacheableBlockCount(
    _In_ UCHAR DriveNumber)
{
    PPC98_DISK_DRIVE DiskDrive;

    DiskDrive = Pc98DiskDriveNumberToDrive(DriveNumber);
    if (!DiskDrive)
        return 1; // Unknown count.

    /*
     * If LBA is supported then the block size will be 64 sectors (32k).
     * If not then the block size is the size of one track.
     */
    if (DiskDrive->Flags & DRIVE_FLAGS_LBA)
        return 64;
    else
        return DiskDrive->Geometry.SectorsPerTrack;
}
