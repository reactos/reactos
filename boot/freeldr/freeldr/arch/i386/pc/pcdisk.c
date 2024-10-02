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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

// #if defined(__i386__) || defined(_M_AMD64)

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(DISK);

/* Enable this line if you want to support multi-drive caching (increases FreeLdr size!) */
// #define CACHE_MULTI_DRIVES

#include <pshpack2.h>

typedef struct
{
    UCHAR       PacketSize;             // 00h - Size of packet (10h or 18h)
    UCHAR       Reserved;               // 01h - Reserved (0)
    USHORT      LBABlockCount;          // 02h - Number of blocks to transfer (max 007Fh for Phoenix EDD)
    USHORT      TransferBufferOffset;   // 04h - Transfer buffer offset (seg:off)
    USHORT      TransferBufferSegment;  //       Transfer buffer segment (seg:off)
    ULONGLONG   LBAStartBlock;          // 08h - Starting absolute block number
//  ULONGLONG   TransferBuffer64;       // 10h - (EDD-3.0, optional) 64-bit flat address of transfer buffer
                                        //       used if DWORD at 04h is FFFFh:FFFFh
                                        //       Commented since some earlier BIOSes refuse to work with
                                        //       such extended structure
} I386_DISK_ADDRESS_PACKET, *PI386_DISK_ADDRESS_PACKET;

typedef struct
{
    UCHAR   PacketSize;     // 00h - Size of packet in bytes (13h)
    UCHAR   MediaType;      // 01h - Boot media type (see #00282)
    UCHAR   DriveNumber;    /* 02h - Drive number:
                             *   00h Floppy image
                             *   80h Bootable hard disk
                             *   81h-FFh Nonbootable or no emulation
                             */
    UCHAR   Controller;     // 03h - CD-ROM controller number
    ULONG   LBAImage;       // 04h - Logical Block Address of disk image to emulate
    USHORT  DeviceSpec;     /* 08h - Device specification (see also #00282)
                             * (IDE) Bit 0:
                             *     Drive is slave instead of master
                             * (SCSI) Bits 7-0:
                             *     LUN and PUN
                             * Bits 15-8:
                             *     Bus number
                             */
    USHORT  Buffer;         // 0Ah - Segment of 3K buffer for caching CD-ROM reads
    USHORT  LoadSeg;        // 0Ch - Load segment for initial boot image.
                            //       If 0000h, load at segment 07C0h.
    USHORT  SectorCount;    // 0Eh - Number of 512-byte virtual sectors to load
                            //       (only valid for AH=4Ch).
    UCHAR   CHSGeometry[3]; /* 10h - Low byte of cylinder count (for INT 13/AH=08h)
                             * 11h - Sector count, high bits of cylinder count (for INT 13/AH=08h)
                             * 12h - Head count (for INT 13/AH=08h)
                             */
    UCHAR   Reserved;
} I386_CDROM_SPEC_PACKET, *PI386_CDROM_SPEC_PACKET;

#include <poppack.h>

typedef struct _PC_DISK_DRIVE
{
    /* Disk geometry (legacy BIOS and INT13 extended) */
    GEOMETRY Geometry;
    EXTENDED_GEOMETRY ExtGeometry;

    /* TRUE when INT 13h extensions are supported */
    BOOLEAN Int13ExtensionsSupported;

    /*
     * 'IsRemovable' flag: TRUE when the drive is removable (e.g. floppy, CD-ROM...).
     * In that case some of the cached information might need to be refreshed regularly.
     */
    BOOLEAN IsRemovable;

#ifdef CACHE_MULTI_DRIVES
    /*
     * 'Initialized' flag: if TRUE then the drive has been initialized;
     * if FALSE then it needs to be initialized; if its high bit is set
     * then there has been an error; don't try to use it.
     */
    BOOLEAN Initialized;
#endif
} PC_DISK_DRIVE, *PPC_DISK_DRIVE;

#ifdef CACHE_MULTI_DRIVES
/* Cache of all possible PC disk drives */
// Maximum number of disks is 0x100, indexed from 0x00 to 0xFF.
static PC_DISK_DRIVE PcDiskDrive[0x100];
#else
/* Cached data for the last-accessed PC disk drive */
// We use a USHORT so that we can initialize it with a drive number that cannot exist
// on the system (they are <= 0xFF), therefore forcing drive caching on first access.
static USHORT LastDriveNumber = 0xFFFF;
static PC_DISK_DRIVE PcDiskDrive;
#endif /* CACHE_MULTI_DRIVES */

/* DISK IO ERROR SUPPORT *****************************************************/

static LONG lReportError = 0; // >= 0: display errors; < 0: hide errors.

LONG DiskReportError(BOOLEAN bShowError)
{
    /* Set the reference count */
    if (bShowError) ++lReportError;
    else            --lReportError;
    return lReportError;
}

static PCSTR DiskGetErrorCodeString(ULONG ErrorCode)
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

    default:    return "unknown error code";
    }
}

static VOID DiskError(PCSTR ErrorString, ULONG ErrorCode)
{
    CHAR ErrorCodeString[200];

    if (lReportError < 0)
        return;

    sprintf(ErrorCodeString, "%s\n\nError Code: 0x%lx\nError: %s",
            ErrorString, ErrorCode, DiskGetErrorCodeString(ErrorCode));

    ERR("%s\n", ErrorCodeString);

    UiMessageBox(ErrorCodeString);
}

/* FUNCTIONS *****************************************************************/

BOOLEAN DiskResetController(UCHAR DriveNumber)
{
    REGS RegsIn, RegsOut;

    WARN("DiskResetController(0x%x) DISK OPERATION FAILED -- RESETTING CONTROLLER\n", DriveNumber);

    /*
     * BIOS Int 13h, function 0 - Reset disk system
     * AH = 00h
     * DL = drive (if bit 7 is set both hard disks and floppy disks reset)
     * Return:
     * AH = status
     * CF clear if successful
     * CF set on error
     */
    RegsIn.b.ah = 0x00;
    RegsIn.b.dl = DriveNumber;

    /* Reset the disk controller */
    Int386(0x13, &RegsIn, &RegsOut);

    return INT386_SUCCESS(RegsOut);
}

static BOOLEAN
DiskIsDriveRemovable(UCHAR DriveNumber)
{
    /*
     * Hard disks use drive numbers >= 0x80 . So if the drive number
     * indicates a hard disk then return FALSE.
     * 0x49 is our magic ramdisk drive, so return FALSE for that too.
     */
    if ((DriveNumber >= 0x80) || (DriveNumber == 0x49))
        return FALSE;

    /* The drive is a floppy diskette so return TRUE */
    return TRUE;
}

static BOOLEAN
DiskInt13ExtensionsSupported(IN UCHAR DriveNumber)
{
    REGS RegsIn, RegsOut;

    /*
     * Some BIOSes report that extended disk access functions are not supported
     * when booting from a CD (e.g. Phoenix BIOS v6.00PG and Insyde BIOS shipping
     * with Intel Macs). Therefore we just return TRUE if we're booting from a CD
     * - we can assume that all El Torito capable BIOSes support INT 13 extensions.
     * We simply detect whether we're booting from CD by checking whether the drive
     * number is >= 0x8A. It's 0x90 on the Insyde BIOS, and 0x9F on most other BIOSes.
     */
    if (DriveNumber >= 0x8A)
        return TRUE;

    /*
     * IBM/MS INT 13 Extensions - INSTALLATION CHECK
     * AH = 41h
     * BX = 55AAh
     * DL = drive (80h-FFh)
     * Return:
     * CF set on error (extensions not supported)
     * AH = 01h (invalid function)
     * CF clear if successful
     * BX = AA55h if installed
     * AH = major version of extensions
     * 01h = 1.x
     * 20h = 2.0 / EDD-1.0
     * 21h = 2.1 / EDD-1.1
     * 30h = EDD-3.0
     * AL = internal use
     * CX = API subset support bitmap
     * DH = extension version (v2.0+ ??? -- not present in 1.x)
     *
     * Bitfields for IBM/MS INT 13 Extensions API support bitmap
     * Bit 0, extended disk access functions (AH=42h-44h,47h,48h) supported
     * Bit 1, removable drive controller functions (AH=45h,46h,48h,49h,INT 15/AH=52h) supported
     * Bit 2, enhanced disk drive (EDD) functions (AH=48h,AH=4Eh) supported
     *        extended drive parameter table is valid
     * Bits 3-15 reserved
     */
    RegsIn.b.ah = 0x41;
    RegsIn.w.bx = 0x55AA;
    RegsIn.b.dl = DriveNumber;

    /* Reset the disk controller */
    Int386(0x13, &RegsIn, &RegsOut);
    if (!INT386_SUCCESS(RegsOut))
    {
        /* CF set on error (extensions not supported) */
        return FALSE;
    }

    if (RegsOut.w.bx != 0xAA55)
    {
        /* BX = AA55h if installed */
        return FALSE;
    }

    if (!(RegsOut.w.cx & 0x0001))
    {
        /*
         * CX = API subset support bitmap.
         * Bit 0, extended disk access functions (AH=42h-44h,47h,48h) supported.
         */
        WARN("Suspicious API subset support bitmap 0x%x on device 0x%lx\n",
             RegsOut.w.cx, DriveNumber);
        return FALSE;
    }

    return TRUE;
}

static BOOLEAN
DiskGetExtendedDriveParameters(
    IN UCHAR DriveNumber,
    IN PPC_DISK_DRIVE DiskDrive,
    OUT PVOID Buffer,
    IN USHORT BufferSize)
{
    REGS RegsIn, RegsOut;
    PUSHORT Ptr = (PUSHORT)(BIOSCALLBUFFER);

    TRACE("DiskGetExtendedDriveParameters(0x%x)\n", DriveNumber);

    if (!DiskDrive->Int13ExtensionsSupported)
        return FALSE;

    /* Initialize transfer buffer */
    *Ptr = BufferSize;

    /*
     * BIOS Int 13h, function 48h - Get drive parameters
     * AH = 48h
     * DL = drive (bit 7 set for hard disk)
     * DS:SI = result buffer
     * Return:
     * CF set on error
     * AH = status (07h)
     * CF clear if successful
     * AH = 00h
     * DS:SI -> result buffer
     */
    RegsIn.b.ah = 0x48;
    RegsIn.b.dl = DriveNumber;
    RegsIn.x.ds = BIOSCALLBUFSEGMENT;   // DS:SI -> result buffer
    RegsIn.w.si = BIOSCALLBUFOFFSET;

    /* Get drive parameters */
    Int386(0x13, &RegsIn, &RegsOut);
    if (!INT386_SUCCESS(RegsOut))
        return FALSE;

    RtlCopyMemory(Buffer, Ptr, BufferSize);

#if DBG
    TRACE("size of buffer:                          %x\n", Ptr[0]);
    TRACE("information flags:                       %x\n", Ptr[1]);
    TRACE("number of physical cylinders on drive:   %u\n", *(PULONG)&Ptr[2]);
    TRACE("number of physical heads on drive:       %u\n", *(PULONG)&Ptr[4]);
    TRACE("number of physical sectors per track:    %u\n", *(PULONG)&Ptr[6]);
    TRACE("total number of sectors on drive:        %I64u\n", *(PULONGLONG)&Ptr[8]);
    TRACE("bytes per sector:                        %u\n", Ptr[12]);
    if (Ptr[0] >= 0x1e)
    {
        // Ptr[13]: offset, Ptr[14]: segment
        TRACE("EDD configuration parameters:            %x:%x\n", Ptr[14], Ptr[13]);
        if (Ptr[13] != 0xffff && Ptr[14] != 0xffff)
        {
            PUCHAR SpecPtr = (PUCHAR)(ULONG_PTR)((Ptr[14] << 4) + Ptr[13]);
            TRACE("SpecPtr:                                 %x\n", SpecPtr);
            TRACE("physical I/O port base address:          %x\n", *(PUSHORT)&SpecPtr[0]);
            TRACE("disk-drive control port address:         %x\n", *(PUSHORT)&SpecPtr[2]);
            TRACE("drive flags:                             %x\n", SpecPtr[4]);
            TRACE("proprietary information:                 %x\n", SpecPtr[5]);
            TRACE("IRQ for drive:                           %u\n", SpecPtr[6]);
            TRACE("sector count for multi-sector transfers: %u\n", SpecPtr[7]);
            TRACE("DMA control:                             %x\n", SpecPtr[8]);
            TRACE("programmed I/O control:                  %x\n", SpecPtr[9]);
            TRACE("drive options:                           %x\n", *(PUSHORT)&SpecPtr[10]);
        }
    }
    if (Ptr[0] >= 0x42)
    {
        TRACE("signature:                             %x\n", Ptr[15]);
    }
#endif

    return TRUE;
}

static BOOLEAN
InitDriveGeometry(
    IN UCHAR DriveNumber,
    IN PPC_DISK_DRIVE DiskDrive)
{
    BOOLEAN Success;
    REGS RegsIn, RegsOut;
    ULONG Cylinders;

    /* Get the extended geometry first */
    DiskDrive->ExtGeometry.Size = sizeof(DiskDrive->ExtGeometry);
    Success = DiskGetExtendedDriveParameters(DriveNumber, DiskDrive,
                                             &DiskDrive->ExtGeometry,
                                             DiskDrive->ExtGeometry.Size);
    if (!Success)
    {
        /* Failed, zero it out */
        RtlZeroMemory(&DiskDrive->ExtGeometry, sizeof(DiskDrive->ExtGeometry));
    }
    else
    {
        TRACE("DiskGetExtendedDriveParameters(0x%x) returned:\n"
              "Cylinders  : 0x%x\n"
              "Heads      : 0x%x\n"
              "Sects/Track: 0x%x\n"
              "Bytes/Sect : 0x%x\n",
              DriveNumber,
              DiskDrive->ExtGeometry.Cylinders,
              DiskDrive->ExtGeometry.Heads,
              DiskDrive->ExtGeometry.SectorsPerTrack,
              DiskDrive->ExtGeometry.BytesPerSector);
    }

    /* Now try the legacy geometry */
    RtlZeroMemory(&DiskDrive->Geometry, sizeof(DiskDrive->Geometry));

    /*
     * BIOS Int 13h, function 08h - Get drive parameters
     * AH = 08h
     * DL = drive (bit 7 set for hard disk)
     * ES:DI = 0000h:0000h to guard against BIOS bugs
     * Return:
     * CF set on error
     * AH = status (07h)
     * CF clear if successful
     * AH = 00h
     * AL = 00h on at least some BIOSes
     * BL = drive type (AT/PS2 floppies only)
     * CH = low eight bits of maximum cylinder number
     * CL = maximum sector number (bits 5-0)
     *      high two bits of maximum cylinder number (bits 7-6)
     * DH = maximum head number
     * DL = number of drives
     * ES:DI -> drive parameter table (floppies only)
     */
    RegsIn.b.ah = 0x08;
    RegsIn.b.dl = DriveNumber;
    RegsIn.w.es = 0x0000;
    RegsIn.w.di = 0x0000;

    /* Get drive parameters */
    Int386(0x13, &RegsIn, &RegsOut);
    if (!INT386_SUCCESS(RegsOut))
    {
        /* We failed, return the result of the previous call (extended geometry) */
        return Success;
    }
    /* OR it with the old result, so that we return TRUE whenever either call succeeded */
    Success |= TRUE;

    Cylinders = (RegsOut.b.cl & 0xC0) << 2;
    Cylinders += RegsOut.b.ch;
    Cylinders++;
    DiskDrive->Geometry.Cylinders = Cylinders;
    DiskDrive->Geometry.Heads = RegsOut.b.dh + 1;
    DiskDrive->Geometry.SectorsPerTrack = RegsOut.b.cl & 0x3F;
    DiskDrive->Geometry.BytesPerSector = 512;   /* Just assume 512 bytes per sector */

    DiskDrive->Geometry.Sectors = (ULONGLONG)DiskDrive->Geometry.Cylinders *
                                             DiskDrive->Geometry.Heads *
                                             DiskDrive->Geometry.SectorsPerTrack;

    TRACE("Regular Int13h(0x%x) returned:\n"
          "Cylinders  : 0x%x\n"
          "Heads      : 0x%x\n"
          "Sects/Track: 0x%x (original 0x%x)\n"
          "Bytes/Sect : 0x%x\n",
          DriveNumber,
          DiskDrive->Geometry.Cylinders,
          DiskDrive->Geometry.Heads,
          DiskDrive->Geometry.SectorsPerTrack, RegsOut.b.cl,
          DiskDrive->Geometry.BytesPerSector);

    return Success;
}

static BOOLEAN
PcDiskDriveInit(
    IN UCHAR DriveNumber,
    IN OUT PPC_DISK_DRIVE DiskDrive)
{
    DiskDrive->IsRemovable = DiskIsDriveRemovable(DriveNumber);

    /*
     * Check to see if it is a fixed disk drive.
     * If so then check to see if INT 13h extensions work.
     * If they do then use them, otherwise default back to BIOS calls.
     */
    DiskDrive->Int13ExtensionsSupported = DiskInt13ExtensionsSupported(DriveNumber);

    if (!InitDriveGeometry(DriveNumber, DiskDrive))
        return FALSE;

    TRACE("\n"
          "DriveNumber: 0x%x\n"
          "IsRemovable              = %s\n"
          "Int13ExtensionsSupported = %s\n",
          DriveNumber,
          DiskDrive->IsRemovable ? "TRUE" : "FALSE",
          DiskDrive->Int13ExtensionsSupported ? "TRUE" : "FALSE");

    return TRUE;
}

static inline
PPC_DISK_DRIVE
PcDiskDriveNumberToDrive(IN UCHAR DriveNumber)
{
#ifdef CACHE_MULTI_DRIVES
    PPC_DISK_DRIVE DiskDrive;

    ASSERT((0 <= DriveNumber) && (DriveNumber < RTL_NUMBER_OF(PcDiskDrive)));

    /* Retrieve a slot */
    DiskDrive = &PcDiskDrive[DriveNumber];

    /* If the drive has not been initialized before... */
    if (!DiskDrive->Initialized)
    {
        /* ... try to initialize it now. */
        if (!PcDiskDriveInit(DriveNumber, DiskDrive))
        {
            /*
             * If we failed, there is no drive at this number
             * and flag it as such (set its high bit).
             */
            DiskDrive->Initialized |= 0x80;
            return NULL;
        }
        DiskDrive->Initialized = TRUE;
    }
    else if (DiskDrive->Initialized & 0x80)
    {
        /*
         * The disk failed to be initialized previously, reset its flag to give
         * it chance to be initialized again later, but just fail for the moment.
         */
        DiskDrive->Initialized = FALSE;
        return NULL;
    }

    return DiskDrive;
#else
    static PC_DISK_DRIVE NewDiskDrive;

    ASSERT((0 <= DriveNumber) && (DriveNumber <= 0xFF));

    /* Update cached information */

    /* If the drive has not been accessed last before... */
    if ((USHORT)DriveNumber != LastDriveNumber)
    {
        /* ... try to (re-)initialize and cache it now. */
        RtlZeroMemory(&NewDiskDrive, sizeof(NewDiskDrive));
        if (!PcDiskDriveInit(DriveNumber, &NewDiskDrive))
        {
            /*
             * If we failed, there is no drive at this number.
             * Keep the last-accessed valid drive cached.
             */
            return NULL;
        }
        /* We succeeded, cache the drive data */
        PcDiskDrive = NewDiskDrive;
        LastDriveNumber = (USHORT)DriveNumber;
    }

    return &PcDiskDrive;
#endif /* CACHE_MULTI_DRIVES */
}

static BOOLEAN
PcDiskReadLogicalSectorsLBA(
    IN UCHAR DriveNumber,
    IN ULONGLONG SectorNumber,
    IN ULONG SectorCount,
    OUT PVOID Buffer)
{
    REGS RegsIn, RegsOut;
    ULONG RetryCount;
    PI386_DISK_ADDRESS_PACKET Packet = (PI386_DISK_ADDRESS_PACKET)(BIOSCALLBUFFER);

    /* Setup disk address packet */
    RtlZeroMemory(Packet, sizeof(*Packet));
    Packet->PacketSize = sizeof(*Packet);
    Packet->Reserved = 0;
    // Packet->LBABlockCount set in the loop.
    Packet->TransferBufferOffset = ((ULONG_PTR)Buffer) & 0x0F;
    Packet->TransferBufferSegment = (USHORT)(((ULONG_PTR)Buffer) >> 4);
    Packet->LBAStartBlock = SectorNumber;

    /*
     * BIOS Int 13h, function 42h - IBM/MS INT 13 Extensions - EXTENDED READ
     * Return:
     * CF clear if successful
     * AH = 00h
     * CF set on error
     * AH = error code
     * Disk address packet's block count field set to the
     * number of blocks successfully transferred.
     */
    RegsIn.b.ah = 0x42;
    RegsIn.b.dl = DriveNumber;          // Drive number in DL (0 - floppy, 0x80 - harddisk)
    RegsIn.x.ds = BIOSCALLBUFSEGMENT;   // DS:SI -> disk address packet
    RegsIn.w.si = BIOSCALLBUFOFFSET;

    /* Retry 3 times */
    for (RetryCount = 0; RetryCount < 3; ++RetryCount)
    {
        /* Restore the number of blocks to transfer, since it gets reset
         * on failure with the number of blocks that were successfully
         * transferred (and which could be zero). */
        Packet->LBABlockCount = (USHORT)SectorCount;
        ASSERT(Packet->LBABlockCount == SectorCount);

        Int386(0x13, &RegsIn, &RegsOut);

        /* If it worked, or if it was a corrected ECC error
         * and the data is still good, return success */
        if (INT386_SUCCESS(RegsOut) || (RegsOut.b.ah == 0x11))
            return TRUE;

        /* It failed, do the next retry */
        DiskResetController(DriveNumber);
    }

    /* If we get here then the read failed */
    DiskError("Disk Read Failed in LBA mode", RegsOut.b.ah);
    ERR("Disk Read Failed in LBA mode: %x (%s) (DriveNumber: 0x%x SectorNumber: %I64d SectorCount: %d)\n",
        RegsOut.b.ah, DiskGetErrorCodeString(RegsOut.b.ah),
        DriveNumber, SectorNumber, SectorCount);

    return FALSE;
}

static BOOLEAN
PcDiskReadLogicalSectorsCHS(
    IN UCHAR DriveNumber,
    IN PPC_DISK_DRIVE DiskDrive,
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
    if (DriveGeometry.SectorsPerTrack == 0 || DriveGeometry.Heads == 0)
        return FALSE;

    while (SectorCount > 0)
    {
        /*
         * Calculate the physical disk offsets.
         * Note: DriveGeometry.SectorsPerTrack < 64
         */
        PhysicalSector = 1 + (UCHAR)(SectorNumber % DriveGeometry.SectorsPerTrack);
        PhysicalHead = (UCHAR)((SectorNumber / DriveGeometry.SectorsPerTrack) % DriveGeometry.Heads);
        PhysicalTrack = (ULONG)((SectorNumber / DriveGeometry.SectorsPerTrack) / DriveGeometry.Heads);

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

        /*
         * BIOS Int 13h, function 2 - Read Disk Sectors
         * AH = 02h
         * AL = number of sectors to read (must be nonzero)
         * CH = low eight bits of cylinder number
         * CL = sector number 1-63 (bits 0-5)
         *      high two bits of cylinder (bits 6-7, hard disk only)
         * DH = head number
         * DL = drive number (bit 7 set for hard disk)
         * ES:BX -> data buffer
         * Return:
         * CF set on error
         * if AH = 11h (corrected ECC error), AL = burst length
         * CF clear if successful
         * AH = status
         * AL = number of sectors transferred
         *  (only valid if CF set for some BIOSes)
         */
        RegsIn.b.ah = 0x02;
        RegsIn.b.al = (UCHAR)NumberOfSectorsToRead;
        RegsIn.b.ch = (PhysicalTrack & 0xFF);
        RegsIn.b.cl = (UCHAR)(PhysicalSector + ((PhysicalTrack & 0x300) >> 2));
        RegsIn.b.dh = PhysicalHead;
        RegsIn.b.dl = DriveNumber;
        RegsIn.w.es = (USHORT)(((ULONG_PTR)Buffer) >> 4);
        RegsIn.w.bx = ((ULONG_PTR)Buffer) & 0x0F;

        /* Perform the read. Retry 3 times. */
        for (RetryCount = 0; RetryCount < 3; ++RetryCount)
        {
            Int386(0x13, &RegsIn, &RegsOut);

            /* If it worked, or if it was a corrected ECC error
             * and the data is still good, break out */
            if (INT386_SUCCESS(RegsOut) || (RegsOut.b.ah == 0x11))
                break;

            /* It failed, do the next retry */
            DiskResetController(DriveNumber);
        }

        /* If we retried 3 times then fail */
        if (RetryCount >= 3)
        {
            DiskError("Disk Read Failed in CHS mode, after retrying 3 times", RegsOut.b.ah);
            ERR("Disk Read Failed in CHS mode, after retrying 3 times: %x (%s) (DriveNumber: 0x%x SectorNumber: %I64d SectorCount: %d)\n",
                RegsOut.b.ah, DiskGetErrorCodeString(RegsOut.b.ah),
                DriveNumber, SectorNumber, SectorCount);
            return FALSE;
        }

        /*
         * I have learned that not all BIOSes return
         * the sector read count in the AL register (at least mine doesn't)
         * even if the sectors were read correctly. So instead
         * of checking the sector read count we will rely solely
         * on the carry flag being set on error.
         */

        Buffer = (PVOID)((ULONG_PTR)Buffer + (NumberOfSectorsToRead * DriveGeometry.BytesPerSector));
        SectorCount -= NumberOfSectorsToRead;
        SectorNumber += NumberOfSectorsToRead;
    }

    return TRUE;
}

BOOLEAN
PcDiskReadLogicalSectors(
    IN UCHAR DriveNumber,
    IN ULONGLONG SectorNumber,
    IN ULONG SectorCount,
    OUT PVOID Buffer)
{
    PPC_DISK_DRIVE DiskDrive;

    TRACE("PcDiskReadLogicalSectors() DriveNumber: 0x%x SectorNumber: %I64d SectorCount: %d Buffer: 0x%x\n",
          DriveNumber, SectorNumber, SectorCount, Buffer);

    /* 16-bit BIOS addressing limitation */
    ASSERT(((ULONG_PTR)Buffer) <= 0xFFFFF);

    DiskDrive = PcDiskDriveNumberToDrive(DriveNumber);
    if (!DiskDrive)
        return FALSE;

    if ((DriveNumber >= 0x80) && DiskDrive->Int13ExtensionsSupported)
    {
        /* LBA is easy, nothing to calculate. Just do the read. */
        TRACE("--> Using LBA\n");
        return PcDiskReadLogicalSectorsLBA(DriveNumber, SectorNumber, SectorCount, Buffer);
    }
    else
    {
        /* LBA is not supported, default to CHS */
        TRACE("--> Using CHS\n");
        return PcDiskReadLogicalSectorsCHS(DriveNumber, DiskDrive, SectorNumber, SectorCount, Buffer);
    }
}

#if defined(__i386__) || defined(_M_AMD64)
VOID __cdecl DiskStopFloppyMotor(VOID)
{
    WRITE_PORT_UCHAR((PUCHAR)0x3F2, 0); // DOR_FDC_ENABLE | DOR_DMA_IO_INTERFACE_ENABLE 0x0C // we changed 0x0C->0 to workaround CORE-16469
}
#endif // defined __i386__ || defined(_M_AMD64)

BOOLEAN
PcDiskGetDriveGeometry(UCHAR DriveNumber, PGEOMETRY Geometry)
{
    PPC_DISK_DRIVE DiskDrive;

    TRACE("PcDiskGetDriveGeometry(0x%x)\n", DriveNumber);

    DiskDrive = PcDiskDriveNumberToDrive(DriveNumber);
    if (!DiskDrive)
        return FALSE;

    /* Try to get the extended geometry first */
    if (DiskDrive->ExtGeometry.Size == sizeof(DiskDrive->ExtGeometry))
    {
        /* Extended geometry has been initialized, return it */
        Geometry->Cylinders = DiskDrive->ExtGeometry.Cylinders;
        Geometry->Heads = DiskDrive->ExtGeometry.Heads;
        Geometry->SectorsPerTrack = DiskDrive->ExtGeometry.SectorsPerTrack;
        Geometry->BytesPerSector = DiskDrive->ExtGeometry.BytesPerSector;
        Geometry->Sectors = DiskDrive->ExtGeometry.Sectors;
    }
    else
    /* Fall back to legacy BIOS geometry */
    {
        *Geometry = DiskDrive->Geometry;
    }

    return TRUE;
}

ULONG
PcDiskGetCacheableBlockCount(UCHAR DriveNumber)
{
    PPC_DISK_DRIVE DiskDrive;

    DiskDrive = PcDiskDriveNumberToDrive(DriveNumber);
    if (!DiskDrive)
        return 1; // Unknown count.

    /*
     * If LBA is supported then the block size will be 64 sectors (32k).
     * If not then the block size is the size of one track.
     */
    if (DiskDrive->Int13ExtensionsSupported)
        return 64;
    else
        return DiskDrive->Geometry.SectorsPerTrack;
}

/* EOF */
