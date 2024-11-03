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
PC98_DISK_DRIVE Pc98DiskDrive[MAX_DRIVES];

/* DISK IO ERROR SUPPORT ******************************************************/

static LONG lReportError = 0; /* >= 0: display errors; < 0: hide errors */

LONG
DiskReportError(BOOLEAN bShowError)
{
    /* Set the reference count */
    if (bShowError)
        ++lReportError;
    else
        --lReportError;
    return lReportError;
}

static PCSTR
DiskGetErrorCodeString(ULONG ErrorCode)
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

static VOID
DiskError(PCSTR ErrorString, ULONG ErrorCode)
{
    CHAR ErrorCodeString[200];

    if (lReportError < 0)
        return;

    RtlStringCbPrintfA(ErrorCodeString, sizeof(ErrorCodeString), "%s\n\nError Code: 0x%lx\nError: %s",
                       ErrorString, ErrorCode, DiskGetErrorCodeString(ErrorCode));

    ERR("%s\n", ErrorCodeString);

    UiMessageBox(ErrorCodeString);
}

/* FUNCTIONS ******************************************************************/

BOOLEAN DiskResetController(IN PPC98_DISK_DRIVE DiskDrive)
{
    REGS Regs;

    if (DiskDrive->Type & DRIVE_FDD)
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
    else if (DiskDrive->Type != (DRIVE_IDE | DRIVE_CDROM))
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

    WARN("DiskResetController(0x%x) DISK OPERATION FAILED -- RESETTING CONTROLLER\n", DiskDrive->DaUa);

    Regs.b.al = DiskDrive->DaUa;
    Int386(0x1B, &Regs, &Regs);
    return INT386_SUCCESS(Regs);
}

VOID Pc98DiskPrepareForReactOS(VOID)
{
    AtaFree();
}

PPC98_DISK_DRIVE
Pc98DiskDriveNumberToDrive(IN UCHAR DriveNumber)
{
    PPC98_DISK_DRIVE DiskDrive;

    ASSERT((0 <= DriveNumber) && (DriveNumber < RTL_NUMBER_OF(Pc98DiskDrive)));

    /* Retrieve a slot */
    DiskDrive = &Pc98DiskDrive[DriveNumber];

    /* The pre-initialization of the BIOS disks was already done in Pc98InitializeBootDevices() */
    if (DiskDrive->Initialized)
        return DiskDrive;
    else
        return NULL;
}

static inline
UCHAR
BytesPerSectorToSectorLengthCode(IN ULONG BytesPerSector)
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

static BOOLEAN
Pc98DiskReadLogicalSectorsLBA(
    IN PPC98_DISK_DRIVE DiskDrive,
    IN ULONGLONG SectorNumber,
    IN ULONG SectorCount,
    OUT PVOID Buffer)
{
    REGS RegsIn, RegsOut;
    ULONG RetryCount;

    if (DiskDrive->Type & DRIVE_IDE && DiskDrive->Type & DRIVE_CDROM)
    {
        return AtaAtapiReadLogicalSectorsLBA(AtaGetDevice(DiskDrive->IdeUnitNumber), SectorNumber, SectorCount, Buffer);
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
        RegsIn.w.bx = DiskDrive->Geometry.BytesPerSector * SectorCount;
        RegsIn.w.cx = SectorNumber & 0xFFFF;
        RegsIn.w.dx = (SectorNumber >> 16) & 0xFFFF;
        RegsIn.w.es = (USHORT)(((ULONG_PTR)Buffer) >> 4);
        RegsIn.w.bp = ((ULONG_PTR)Buffer) & 0x0F;

        /* Retry 3 times */
        for (RetryCount = 0; RetryCount < 3; RetryCount++)
        {
            Int386(0x1B, &RegsIn, &RegsOut);

            /* If it worked, or if it was a corrected ECC error
             * and the data is still good, return success */
            if (INT386_SUCCESS(RegsOut) || (RegsOut.b.ah == 0x08))
                return TRUE;

            /* It failed, do the next retry */
            DiskResetController(DiskDrive);
        }
    }

    /* If we get here then the read failed */
    DiskError("Disk Read Failed in LBA mode", RegsOut.b.ah);
    ERR("Disk Read Failed in LBA mode: %x (%s) (DriveNumber: 0x%x SectorNumber: %I64d SectorCount: %d)\n",
        RegsOut.b.ah, DiskGetErrorCodeString(RegsOut.b.ah),
        DiskDrive->DaUa, SectorNumber, SectorCount);

    return FALSE;
}

static BOOLEAN
Pc98DiskReadLogicalSectorsCHS(
    IN PPC98_DISK_DRIVE DiskDrive,
    IN ULONGLONG SectorNumber,
    IN ULONG SectorCount,
    OUT PVOID Buffer)
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
        if (DiskDrive->Type & DRIVE_FDD)
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

        if (DiskDrive->Type & DRIVE_FDD)
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
            RegsIn.w.es = (USHORT)(((ULONG_PTR)Buffer) >> 4);
            RegsIn.w.bp = ((ULONG_PTR)Buffer) & 0x0F;
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
            RegsIn.w.es = (USHORT)(((ULONG_PTR)Buffer) >> 4);
            RegsIn.w.bp = ((ULONG_PTR)Buffer) & 0x0F;
        }

        /* Perform the read. Retry 3 times. */
        for (RetryCount = 0; RetryCount < 3; RetryCount++)
        {
            Int386(0x1B, &RegsIn, &RegsOut);

            /* If it worked break out */
            if (INT386_SUCCESS(RegsOut))
            {
                break;
            }
            /* If it was a corrected ECC error then the data is still good */
            else if (RegsOut.b.ah == 0x08)
            {
                break;
            }
            /* If it failed then do the next retry */
            else
            {
                DiskResetController(DiskDrive);
                continue;
            }
        }

        /* If we retried 3 times then fail */
        if (RetryCount >= 3)
        {
            DiskError("Disk Read Failed in CHS mode, after retrying 3 times", RegsOut.b.ah);
            ERR("Disk Read Failed in CHS mode, after retrying 3 times: %x (%s) (DriveNumber: 0x%x SectorNumber: %I64d SectorCount: %d)\n",
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

static BOOLEAN
InitScsiDrive(
    IN UCHAR DaUa,
    IN OUT PPC98_DISK_DRIVE DiskDrive)
{
    REGS RegsIn, RegsOut;
    UCHAR UnitAddress = DaUa & 0x0F;
    USHORT DiskEquipment = *(PUCHAR)MEM_DISK_EQUIPS;
    ULONG ScsiParameters = *(PULONG)(MEM_SCSI_TABLE + UnitAddress * sizeof(ULONG));
    UCHAR DeviceType;

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
        {
            DiskDrive->Initialized = FALSE;
            return FALSE;
        }

        DiskDrive->Geometry.Cylinders = RegsOut.w.cx;
        DiskDrive->Geometry.Heads = RegsOut.b.dh;
        DiskDrive->Geometry.SectorsPerTrack = RegsOut.b.dl;
        DiskDrive->Geometry.BytesPerSector = RegsOut.w.bx;
        DiskDrive->LBASupported = FALSE;
        DiskDrive->IsRemovable = FALSE;
    }
    /* Other devices */
    else if (ScsiParameters)
    {
        DeviceType = ScsiParameters & 0x1F;
        switch (DeviceType)
        {
            case 0x05:
                /* CD-ROM */
                DiskDrive->Geometry.Cylinders = 0xFFFF;
                DiskDrive->Geometry.Heads = 0xFFFF;
                DiskDrive->Geometry.SectorsPerTrack = 0xFFFF;
                DiskDrive->Geometry.BytesPerSector = 2048;
                DiskDrive->Type = DRIVE_CDROM;
                DiskDrive->LBASupported = TRUE;
                DiskDrive->IsRemovable = TRUE;
                break;

            case 0x07:
                /* Magneto-optical drive */
                DiskDrive->Geometry.Cylinders = 0xFFFF;
                DiskDrive->Geometry.Heads = 8;
                DiskDrive->Geometry.SectorsPerTrack = 32;
                DiskDrive->Geometry.BytesPerSector = 512;
                DiskDrive->Type = DRIVE_MO;
                DiskDrive->LBASupported = TRUE;
                DiskDrive->IsRemovable = TRUE;
                break;

            default:
                DiskDrive->Initialized = FALSE;
                return FALSE;
        }
    }
    else
    {
        DiskDrive->Initialized = FALSE;
        return FALSE;
    }

    DiskDrive->Geometry.Sectors = (ULONGLONG)DiskDrive->Geometry.Cylinders *
                                             DiskDrive->Geometry.Heads *
                                             DiskDrive->Geometry.SectorsPerTrack;

    DiskDrive->DaUa = DaUa;
    DiskDrive->Type |= DRIVE_SCSI;
    DiskDrive->Initialized = TRUE;

    TRACE("InitScsiDrive(0x%x) returned:\n"
          "Cylinders  : 0x%x\n"
          "Heads      : 0x%x\n"
          "Sects/Track: 0x%x\n"
          "Bytes/Sect : 0x%x\n",
          DaUa,
          DiskDrive->Geometry.Cylinders,
          DiskDrive->Geometry.Heads,
          DiskDrive->Geometry.SectorsPerTrack,
          DiskDrive->Geometry.BytesPerSector);

    return TRUE;
}

static BOOLEAN
InitIdeDrive(
    IN UCHAR UnitNumber,
    IN OUT PPC98_DISK_DRIVE DiskDrive)
{
    PDEVICE_UNIT DeviceUnit = AtaGetDevice(UnitNumber);

    /* We work directly only with ATAPI drives because BIOS has ATA support */
    if (DeviceUnit && DeviceUnit->Flags & ATA_DEVICE_ATAPI)
    {
        DiskDrive->Geometry.Cylinders = DeviceUnit->Cylinders;
        DiskDrive->Geometry.Heads = DeviceUnit->Heads;
        DiskDrive->Geometry.SectorsPerTrack = DeviceUnit->Sectors;
        DiskDrive->Geometry.BytesPerSector = DeviceUnit->SectorSize;
        DiskDrive->Geometry.Sectors = DeviceUnit->TotalSectors;

        DiskDrive->DaUa = 0xFF;
        DiskDrive->IdeUnitNumber = UnitNumber;
        DiskDrive->Type = DRIVE_IDE | DRIVE_CDROM;
        DiskDrive->LBASupported = TRUE;
        DiskDrive->IsRemovable = TRUE;
        DiskDrive->Initialized = TRUE;

        TRACE("InitIdeDrive(0x%x) returned:\n"
              "Cylinders  : 0x%x\n"
              "Heads      : 0x%x\n"
              "Sects/Track: 0x%x\n"
              "Bytes/Sect : 0x%x\n",
              UnitNumber,
              DiskDrive->Geometry.Cylinders,
              DiskDrive->Geometry.Heads,
              DiskDrive->Geometry.SectorsPerTrack,
              DiskDrive->Geometry.BytesPerSector);

        return TRUE;
    }

    DiskDrive->Initialized = FALSE;
    return FALSE;
}

static BOOLEAN
InitHardDrive(
    IN UCHAR DaUa,
    IN OUT PPC98_DISK_DRIVE DiskDrive)
{
    REGS RegsIn, RegsOut;

    /* Int 1Bh AH=8Eh
     * DISK BIOS - Set half-height operation mode
     *
     * Call with:
     * AL - drive number
     */
    RegsIn.b.al = DaUa;
    RegsIn.b.ah = 0x8E;
    Int386(0x1B, &RegsIn, &RegsOut);

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
    {
        DiskDrive->Initialized = FALSE;
        return FALSE;
    }

    DiskDrive->Geometry.Cylinders = RegsOut.w.cx;
    DiskDrive->Geometry.Heads = RegsOut.b.dh;
    DiskDrive->Geometry.SectorsPerTrack = RegsOut.b.dl;
    DiskDrive->Geometry.BytesPerSector = RegsOut.w.bx;

    DiskDrive->Geometry.Sectors = (ULONGLONG)DiskDrive->Geometry.Cylinders *
                                             DiskDrive->Geometry.Heads *
                                             DiskDrive->Geometry.SectorsPerTrack;

    DiskDrive->DaUa = DaUa;
    DiskDrive->Type = DRIVE_IDE;
    DiskDrive->LBASupported = FALSE;
    DiskDrive->IsRemovable = FALSE;
    DiskDrive->Initialized = TRUE;

    TRACE("InitHardDrive(0x%x) returned:\n"
          "Cylinders  : 0x%x\n"
          "Heads      : 0x%x\n"
          "Sects/Track: 0x%x\n"
          "Bytes/Sect : 0x%x\n",
          DaUa,
          DiskDrive->Geometry.Cylinders,
          DiskDrive->Geometry.Heads,
          DiskDrive->Geometry.SectorsPerTrack,
          DiskDrive->Geometry.BytesPerSector);

    return TRUE;
}

static BOOLEAN
InitFloppyDrive(
    IN UCHAR DaUa,
    IN OUT PPC98_DISK_DRIVE DiskDrive)
{
    REGS RegsIn, RegsOut;
    USHORT BytesPerSector;
    UCHAR DeviceAddress = DaUa & 0xF0;

    /* There's no way to obtain floppy disk geometry in BIOS */

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
    {
        DiskDrive->Initialized = FALSE;
        return FALSE;
    }

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
            DiskDrive->Initialized = FALSE;
            return FALSE;
    }

    DiskDrive->Geometry.BytesPerSector = BytesPerSector;
    DiskDrive->Geometry.Sectors = (ULONGLONG)DiskDrive->Geometry.Cylinders *
                                             DiskDrive->Geometry.Heads *
                                             DiskDrive->Geometry.SectorsPerTrack;

    DiskDrive->DaUa = DaUa;
    DiskDrive->Type = DRIVE_FDD;
    DiskDrive->LBASupported = FALSE;
    DiskDrive->IsRemovable = TRUE;
    DiskDrive->Initialized = TRUE;

    TRACE("InitFloppyDrive(0x%x) returned:\n"
          "Cylinders  : 0x%x\n"
          "Heads      : 0x%x\n"
          "Sects/Track: 0x%x\n"
          "Bytes/Sect : 0x%x\n",
          DaUa,
          DiskDrive->Geometry.Cylinders,
          DiskDrive->Geometry.Heads,
          DiskDrive->Geometry.SectorsPerTrack,
          DiskDrive->Geometry.BytesPerSector);

    return TRUE;
}

/* We emulate PC BIOS drive numbers here */
BOOLEAN
Pc98InitializeBootDevices(VOID)
{
    PPC98_DISK_DRIVE DiskDrive;
    UCHAR FakeFloppyDriveNumber = 0x30;
    UCHAR FakeHardDriveDriveNumber = 0x80;
    UCHAR FakeCdRomDriveNumber = 0xE0;
    USHORT DiskEquipment = *(PUSHORT)MEM_DISK_EQUIP & ~(*(PUCHAR)MEM_RDISK_EQUIP);
    UCHAR IdeDetectedCount;
    UCHAR i;

    TRACE("Pc98InitializeBootDevices()\n");

    RtlZeroMemory(&Pc98DiskDrive, sizeof(Pc98DiskDrive));

    /*
     * Map DA/UA to drive number, i.e.
     * 0x90 -> 0x30
     * 0x80 -> 0x80
     * 0xA0 -> 0x81, etc.
     */

    /* Map floppies */

    for (i = 0; i < 4; i++)
    {
        DiskDrive = &Pc98DiskDrive[FakeFloppyDriveNumber];
        if (FIRSTBYTE(DiskEquipment) & (1 << i))
        {
            if (InitFloppyDrive(0x30 + i, DiskDrive) || InitFloppyDrive(0xB0 + i, DiskDrive) ||
                InitFloppyDrive(0x90 + i, DiskDrive) || InitFloppyDrive(0x10 + i, DiskDrive))
                ++FakeFloppyDriveNumber;
        }
    }

    for (i = 0; i < 4; i++)
    {
        DiskDrive = &Pc98DiskDrive[FakeFloppyDriveNumber];
        if (FIRSTBYTE(DiskEquipment) & (16 << i))
        {
            if (InitFloppyDrive(0x50 + i, DiskDrive))
                ++FakeFloppyDriveNumber;
        }
    }

    for (i = 0; i < 4; i++)
    {
        DiskDrive = &Pc98DiskDrive[FakeFloppyDriveNumber];
        if (SECONDBYTE(DiskEquipment) & (16 << i))
        {
            if (InitFloppyDrive(0x70 + i, DiskDrive) || InitFloppyDrive(0xF0 + i, DiskDrive))
                ++FakeFloppyDriveNumber;
        }
    }

    /* Map IDE/SASI drives */

    for (i = 0; i < 4; i++)
    {
        DiskDrive = &Pc98DiskDrive[FakeHardDriveDriveNumber];
        if (InitHardDrive(0x80 + i, DiskDrive) || InitHardDrive(0x00 + i, DiskDrive))
            ++FakeHardDriveDriveNumber;
    }

    AtaInit(&IdeDetectedCount);
    for (i = 0; i <= IdeDetectedCount; i++)
    {
        DiskDrive = &Pc98DiskDrive[FakeCdRomDriveNumber];
        if (InitIdeDrive(i, DiskDrive))
            ++FakeCdRomDriveNumber;
    }

    /* Map SCSI drives */

    for (i = 0; i < 7; i++)
    {
        DiskDrive = &Pc98DiskDrive[FakeHardDriveDriveNumber];
        if (InitScsiDrive(0xA0 + i, DiskDrive) || InitScsiDrive(0x20 + i, DiskDrive))
        {
            if (DiskDrive->Type & DRIVE_CDROM || DiskDrive->Type & DRIVE_MO)
            {
                /* Move to CD-ROM area */
                Pc98DiskDrive[FakeCdRomDriveNumber] = *DiskDrive;
                RtlZeroMemory(DiskDrive, sizeof(PC98_DISK_DRIVE));
                ++FakeCdRomDriveNumber;
            }
            else
            {
                ++FakeHardDriveDriveNumber;
            }
        }
    }

#if 1
    // Ugly HACK: Force ISO boot
    // FIXME: Fill ARC disk blocks completely
    // to allow usage of CD-ROM root path (See floppy_pc98.ini).
    FrldrBootDrive = 0xE0;
    FrldrBootPartition = 0xFF;
#else
    /* Reassign boot drive */
    for (i = 0; i < MAX_DRIVES - 1; i++)
    {
        DiskDrive = &Pc98DiskDrive[i];
        if (DiskDrive->Initialized && DiskDrive->DaUa == FrldrBootDrive)
        {
            TRACE("Boot drive: old 0x%x, new 0x%x\n", FrldrBootDrive, i);
            FrldrBootDrive = i;
            break;
        }
    }
#endif

    /* Call PC version */
    return PcInitializeBootDevices();
}

BOOLEAN
Pc98DiskReadLogicalSectors(
    IN UCHAR DriveNumber,
    IN ULONGLONG SectorNumber,
    IN ULONG SectorCount,
    OUT PVOID Buffer)
{
    PPC98_DISK_DRIVE DiskDrive;

    TRACE("Pc98DiskReadLogicalSectors() DriveNumber: 0x%x SectorNumber: %I64d SectorCount: %d Buffer: 0x%x\n",
          DriveNumber, SectorNumber, SectorCount, Buffer);

    /* 16-bit BIOS addressing limitation */
    ASSERT(((ULONG_PTR)Buffer) <= 0xFFFFF);

    DiskDrive = Pc98DiskDriveNumberToDrive(DriveNumber);
    if (!DiskDrive)
        return FALSE;

    if (DiskDrive->LBASupported)
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
Pc98DiskGetDriveGeometry(UCHAR DriveNumber, PGEOMETRY Geometry)
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
Pc98DiskGetCacheableBlockCount(UCHAR DriveNumber)
{
    PPC98_DISK_DRIVE DiskDrive;

    DiskDrive = Pc98DiskDriveNumberToDrive(DriveNumber);
    if (!DiskDrive)
        return 1; // Unknown count.

    /*
     * If LBA is supported then the block size will be 64 sectors (32k).
     * If not then the block size is the size of one track.
     */
    if (DiskDrive->LBASupported)
        return 64;
    else
        return DiskDrive->Geometry.SectorsPerTrack;
}
