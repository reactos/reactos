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

#define NDEBUG
#include <debug.h>

DBG_DEFAULT_CHANNEL(DISK);

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

static BOOLEAN PcDiskReadLogicalSectorsLBA(UCHAR DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer)
{
    REGS RegsIn, RegsOut;
    ULONG RetryCount;
    PI386_DISK_ADDRESS_PACKET Packet = (PI386_DISK_ADDRESS_PACKET)(BIOSCALLBUFFER);

    TRACE("PcDiskReadLogicalSectorsLBA() DriveNumber: 0x%x SectorNumber: %I64d SectorCount: %d Buffer: 0x%x\n", DriveNumber, SectorNumber, SectorCount, Buffer);
    ASSERT(((ULONG_PTR)Buffer) <= 0xFFFFF);

    /* Setup disk address packet */
    RtlZeroMemory(Packet, sizeof(*Packet));
    Packet->PacketSize = sizeof(*Packet);
    Packet->Reserved = 0;
    Packet->LBABlockCount = (USHORT)SectorCount;
    ASSERT(Packet->LBABlockCount == SectorCount);
    Packet->TransferBufferOffset = ((ULONG_PTR)Buffer) & 0x0F;
    Packet->TransferBufferSegment = (USHORT)(((ULONG_PTR)Buffer) >> 4);
    Packet->LBAStartBlock = SectorNumber;

    /*
     * BIOS int 0x13, function 42h - IBM/MS INT 13 Extensions - EXTENDED READ
     * Return:
     * CF clear if successful
     * AH = 00h
     * CF set on error
     * AH = error code
     * Disk address packet's block count field set to the
     * number of blocks successfully transferred.
     */
    RegsIn.b.ah = 0x42;                 // Subfunction 42h
    RegsIn.b.dl = DriveNumber;          // Drive number in DL (0 - floppy, 0x80 - harddisk)
    RegsIn.x.ds = BIOSCALLBUFSEGMENT;   // DS:SI -> disk address packet
    RegsIn.w.si = BIOSCALLBUFOFFSET;

    /* Retry 3 times */
    for (RetryCount=0; RetryCount<3; RetryCount++)
    {
        Int386(0x13, &RegsIn, &RegsOut);

        /* If it worked return TRUE */
        if (INT386_SUCCESS(RegsOut))
        {
            return TRUE;
        }
        /* If it was a corrected ECC error then the data is still good */
        else if (RegsOut.b.ah == 0x11)
        {
            return TRUE;
        }
        /* If it failed then do the next retry */
        else
        {
            DiskResetController(DriveNumber);
            continue;
        }
    }

    /* If we get here then the read failed */
    ERR("Disk Read Failed in LBA mode: %x (DriveNumber: 0x%x SectorNumber: %I64d SectorCount: %d)\n", RegsOut.b.ah, DriveNumber, SectorNumber, SectorCount);

    return FALSE;
}

static BOOLEAN PcDiskReadLogicalSectorsCHS(UCHAR DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer)
{
    UCHAR PhysicalSector;
    UCHAR PhysicalHead;
    ULONG PhysicalTrack;
    GEOMETRY DriveGeometry;
    ULONG NumberOfSectorsToRead;
    REGS RegsIn, RegsOut;
    ULONG RetryCount;

    TRACE("PcDiskReadLogicalSectorsCHS()\n");

    /* Get the drive geometry */
    if (!MachDiskGetDriveGeometry(DriveNumber, &DriveGeometry) ||
        DriveGeometry.Sectors == 0 ||
        DriveGeometry.Heads == 0)
    {
        return FALSE;
    }

    while (SectorCount)
    {
        /*
         * Calculate the physical disk offsets.
         * Note: DriveGeometry.Sectors < 64
         */
        PhysicalSector = 1 + (UCHAR)(SectorNumber % DriveGeometry.Sectors);
        PhysicalHead = (UCHAR)((SectorNumber / DriveGeometry.Sectors) % DriveGeometry.Heads);
        PhysicalTrack = (ULONG)((SectorNumber / DriveGeometry.Sectors) / DriveGeometry.Heads);

        /* Calculate how many sectors we need to read this round */
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

        /* Make sure the read is within the geometry boundaries */
        if ((PhysicalHead >= DriveGeometry.Heads) ||
            (PhysicalTrack >= DriveGeometry.Cylinders) ||
            ((NumberOfSectorsToRead + PhysicalSector) > (DriveGeometry.Sectors + 1)) ||
            (PhysicalSector > DriveGeometry.Sectors))
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
        for (RetryCount=0; RetryCount<3; RetryCount++)
        {
            Int386(0x13, &RegsIn, &RegsOut);

            /* If it worked break out */
            if (INT386_SUCCESS(RegsOut))
            {
                break;
            }
            /* If it was a corrected ECC error then the data is still good */
            else if (RegsOut.b.ah == 0x11)
            {
                break;
            }
            /* If it failed the do the next retry */
            else
            {
                DiskResetController(DriveNumber);
                continue;
            }
        }

        /* If we retried 3 times then fail */
        if (RetryCount >= 3)
        {
            ERR("Disk Read Failed in CHS mode, after retrying 3 times: %x\n", RegsOut.b.ah);
            return FALSE;
        }

        // I have learned that not all BIOSes return
        // the sector read count in the AL register (at least mine doesn't)
        // even if the sectors were read correctly. So instead
        // of checking the sector read count we will rely solely
        // on the carry flag being set on error

        Buffer = (PVOID)((ULONG_PTR)Buffer + (NumberOfSectorsToRead * DriveGeometry.BytesPerSector));
        SectorCount -= NumberOfSectorsToRead;
        SectorNumber += NumberOfSectorsToRead;
    }

    return TRUE;
}

static BOOLEAN DiskInt13ExtensionsSupported(UCHAR DriveNumber)
{
    static UCHAR LastDriveNumber = 0xff;
    static BOOLEAN LastSupported;
    REGS RegsIn, RegsOut;

    TRACE("DiskInt13ExtensionsSupported()\n");

    if (DriveNumber == LastDriveNumber)
    {
        TRACE("Using cached value %s for drive 0x%x\n",
              LastSupported ? "TRUE" : "FALSE", DriveNumber);
        return LastSupported;
    }

    /*
     * Some BIOSes report that extended disk access functions are not supported
     * when booting from a CD (e.g. Phoenix BIOS v6.00PG and Insyde BIOS shipping
     * with Intel Macs). Therefore we just return TRUE if we're booting from a CD -
     * we can assume that all El Torito capable BIOSes support INT 13 extensions.
     * We simply detect whether we're booting from CD by checking whether the drive
     * number is >= 0x8A. It's 0x90 on the Insyde BIOS, and 0x9F on most other BIOSes.
     */
    if (DriveNumber >= 0x8A)
    {
        LastSupported = TRUE;
        return TRUE;
    }

    LastDriveNumber = DriveNumber;

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
        LastSupported = FALSE;
        return FALSE;
    }

    if (RegsOut.w.bx != 0xAA55)
    {
        /* BX = AA55h if installed */
        LastSupported = FALSE;
        return FALSE;
    }

    if (!(RegsOut.w.cx & 0x0001))
    {
        /*
         * CX = API subset support bitmap.
         * Bit 0, extended disk access functions (AH=42h-44h,47h,48h) supported.
         */
        DbgPrint("Suspicious API subset support bitmap 0x%x on device 0x%lx\n",
                 RegsOut.w.cx, DriveNumber);
        LastSupported = FALSE;
        return FALSE;
    }

    LastSupported = TRUE;
    return TRUE;
}

BOOLEAN PcDiskReadLogicalSectors(UCHAR DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer)
{
    BOOLEAN ExtensionsSupported;

    TRACE("PcDiskReadLogicalSectors() DriveNumber: 0x%x SectorNumber: %I64d SectorCount: %d Buffer: 0x%x\n",
          DriveNumber, SectorNumber, SectorCount, Buffer);

    /*
     * Check to see if it is a fixed disk drive.
     * If so then check to see if Int13 extensions work.
     * If they do then use them, otherwise default back to BIOS calls.
     */
    ExtensionsSupported = DiskInt13ExtensionsSupported(DriveNumber);

    if ((DriveNumber >= 0x80) && ExtensionsSupported)
    {
        TRACE("Using Int 13 Extensions for read. DiskInt13ExtensionsSupported(%d) = %s\n", DriveNumber, ExtensionsSupported ? "TRUE" : "FALSE");

        /* LBA is easy, nothing to calculate. Just do the read. */
        return PcDiskReadLogicalSectorsLBA(DriveNumber, SectorNumber, SectorCount, Buffer);
    }
    else
    {
        /* LBA is not supported default to the CHS calls */
        return PcDiskReadLogicalSectorsCHS(DriveNumber, SectorNumber, SectorCount, Buffer);
    }

    return TRUE;
}

VOID DiskStopFloppyMotor(VOID)
{
    WRITE_PORT_UCHAR((PUCHAR)0x3F2, 0);
}

BOOLEAN DiskGetExtendedDriveParameters(UCHAR DriveNumber, PVOID Buffer, USHORT BufferSize)
{
    REGS RegsIn, RegsOut;
    PUSHORT Ptr = (PUSHORT)(BIOSCALLBUFFER);

    TRACE("DiskGetExtendedDriveParameters()\n");

    if (!DiskInt13ExtensionsSupported(DriveNumber))
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

    memcpy(Buffer, Ptr, BufferSize);

#if DBG
    TRACE("size of buffer:                          %x\n", Ptr[0]);
    TRACE("information flags:                       %x\n", Ptr[1]);
    TRACE("number of physical cylinders on drive:   %u\n", *(PULONG)&Ptr[2]);
    TRACE("number of physical heads on drive:       %u\n", *(PULONG)&Ptr[4]);
    TRACE("number of physical sectors per track:    %u\n", *(PULONG)&Ptr[6]);
    TRACE("total number of sectors on drive:        %I64u\n", *(unsigned long long*)&Ptr[8]);
    TRACE("bytes per sector:                        %u\n", Ptr[12]);
    if (Ptr[0] >= 0x1e)
    {
        TRACE("EED configuration parameters:            %x:%x\n", Ptr[13], Ptr[14]);
        if (Ptr[13] != 0xffff && Ptr[14] != 0xffff)
        {
           PUCHAR SpecPtr = (PUCHAR)(ULONG_PTR)((Ptr[13] << 4) + Ptr[14]);
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

BOOLEAN
PcDiskGetDriveGeometry(UCHAR DriveNumber, PGEOMETRY Geometry)
{
    EXTENDED_GEOMETRY ExtGeometry;
    REGS RegsIn, RegsOut;
    ULONG Cylinders;

    TRACE("DiskGetDriveGeometry()\n");

    /* Try to get the extended geometry first */
    ExtGeometry.Size = sizeof(ExtGeometry);
    if (DiskGetExtendedDriveParameters(DriveNumber, &ExtGeometry, ExtGeometry.Size))
    {
        Geometry->Cylinders = ExtGeometry.Cylinders;
        Geometry->Heads = ExtGeometry.Heads;
        Geometry->Sectors = ExtGeometry.SectorsPerTrack;
        Geometry->BytesPerSector = ExtGeometry.BytesPerSector;
        return TRUE;
    }

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
        return FALSE;

    Cylinders = (RegsOut.b.cl & 0xC0) << 2;
    Cylinders += RegsOut.b.ch;
    Cylinders++;
    Geometry->Cylinders = Cylinders;
    Geometry->Heads = RegsOut.b.dh + 1;
    Geometry->Sectors = RegsOut.b.cl & 0x3F;
    Geometry->BytesPerSector = 512;     /* Just assume 512 bytes per sector */

    return TRUE;
}

ULONG
PcDiskGetCacheableBlockCount(UCHAR DriveNumber)
{
    GEOMETRY Geometry;

    /* If LBA is supported then the block size will be 64 sectors (32k)
     * If not then the block size is the size of one track. */
    if (DiskInt13ExtensionsSupported(DriveNumber))
    {
        return 64;
    }
    /* Get the disk geometry. If this fails then we will
     * just return 1 sector to be safe. */
    else if (! PcDiskGetDriveGeometry(DriveNumber, &Geometry))
    {
        return 1;
    }
    else
    {
        return Geometry.Sectors;
    }
}


static BOOLEAN
FallbackDiskIsCdRomDrive(UCHAR DriveNumber)
{
    MASTER_BOOT_RECORD MasterBootRecord;

    TRACE("FallbackDiskIsCdRomDrive(0x%x)\n", DriveNumber);

    /* CD-ROM drive numbers are always > 0x80 */
    if (DriveNumber <= 0x80)
        return FALSE;

    /*
     * We suppose that a CD-ROM does not have a MBR
     * (not always true: example of the Hybrid USB-ISOs).
     */
    return !DiskReadBootRecord(DriveNumber, 0, &MasterBootRecord);
}

BOOLEAN DiskIsCdRomDrive(UCHAR DriveNumber)
{
    REGS RegsIn, RegsOut;
    PI386_CDROM_SPEC_PACKET Packet = (PI386_CDROM_SPEC_PACKET)(BIOSCALLBUFFER);

    TRACE("DiskIsCdRomDrive(0x%x)\n", DriveNumber);

    /* CD-ROM drive numbers are always > 0x80 */
    if (DriveNumber <= 0x80)
        return FALSE;

    /* Setup disk address packet */
    RtlZeroMemory(Packet, sizeof(*Packet));
    Packet->PacketSize = sizeof(*Packet);

    /*
     * BIOS Int 13h, function 4B01h - Bootable CD-ROM - Get Disk Emulation Status
     * AX = 4B01h
     * DL = drive number
     * DS:SI -> empty specification packet
     * Return:
     * CF clear if successful
     * CF set on error
     * AX = return codes
     * DS:SI specification packet filled
     */
    RegsIn.w.ax = 0x4B01;
    RegsIn.b.dl = DriveNumber;
    RegsIn.x.ds = BIOSCALLBUFSEGMENT;   // DS:SI -> specification packet
    RegsIn.w.si = BIOSCALLBUFOFFSET;

    Int386(0x13, &RegsIn, &RegsOut);

    // return (INT386_SUCCESS(RegsOut) && (Packet->DriveNumber == DriveNumber));
    /*
     * If the simple test failed, try to use the fallback code,
     * but we can be on *very* thin ice.
     */
    if (!INT386_SUCCESS(RegsOut) || (Packet->DriveNumber != DriveNumber))
        return FallbackDiskIsCdRomDrive(DriveNumber);
    else
        return TRUE;
}

BOOLEAN
PcDiskGetBootPath(OUT PCHAR BootPath, IN ULONG Size)
{
    // FIXME: Keep it there, or put it in DiskGetBootPath?
    // Or, abstract the notion of network booting to make
    // sense for other platforms than the PC (and this idea
    // already exists), then we would need to check whether
    // we were booting from network (and: PC --> PXE, etc...)
    // and if so, set the correct ARC path. But then this new
    // logic could be moved back to DiskGetBootPath...

    if (*FrldrBootPath)
    {
        /* Copy back the buffer */
        if (Size < strlen(FrldrBootPath) + 1)
            return FALSE;
        strncpy(BootPath, FrldrBootPath, Size);
        return TRUE;
    }

    // FIXME! FIXME! Do this in some drive recognition procedure!!!!
    if (PxeInit())
    {
        strcpy(BootPath, "net(0)");
        return TRUE;
    }
    return DiskGetBootPath(BootPath, Size);
}

/* EOF */
