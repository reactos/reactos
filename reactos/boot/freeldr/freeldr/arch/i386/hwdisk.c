/*
 *  FreeLoader
 *
 *  Copyright (C) 2003, 2004  Eric Kohl
 *  Copyright (C) 2009  Hervé Poussineau
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

#include <freeldr.h>

#define NDEBUG
#include <debug.h>

DBG_DEFAULT_CHANNEL(HWDETECT);

typedef struct tagDISKCONTEXT
{
    UCHAR DriveNumber;
    ULONG SectorSize;
    ULONGLONG SectorOffset;
    ULONGLONG SectorCount;
    ULONGLONG SectorNumber;
} DISKCONTEXT;

extern ULONG reactos_disk_count;
extern ARC_DISK_SIGNATURE reactos_arc_disk_info[];
extern CHAR reactos_arc_strings[32][256];

static CHAR Hex[] = "0123456789abcdef";
UCHAR PcBiosDiskCount = 0;
CHAR PcDiskIdentifier[32][20];


static LONG DiskClose(ULONG FileId)
{
    DISKCONTEXT* Context = FsGetDeviceSpecific(FileId);

    FrLdrTempFree(Context, TAG_HW_DISK_CONTEXT);
    return ESUCCESS;
}

static LONG DiskGetFileInformation(ULONG FileId, FILEINFORMATION* Information)
{
    DISKCONTEXT* Context = FsGetDeviceSpecific(FileId);

    RtlZeroMemory(Information, sizeof(FILEINFORMATION));
    Information->EndingAddress.QuadPart = (Context->SectorOffset + Context->SectorCount) * Context->SectorSize;
    Information->CurrentAddress.QuadPart = (Context->SectorOffset + Context->SectorNumber) * Context->SectorSize;

    return ESUCCESS;
}

static LONG DiskOpen(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
{
    DISKCONTEXT* Context;
    UCHAR DriveNumber;
    ULONG DrivePartition, SectorSize;
    ULONGLONG SectorOffset = 0;
    ULONGLONG SectorCount = 0;
    PARTITION_TABLE_ENTRY PartitionTableEntry;
    CHAR FileName[1];

    if (!DissectArcPath(Path, FileName, &DriveNumber, &DrivePartition))
        return EINVAL;

    if (DrivePartition == 0xff)
    {
        /* This is a CD-ROM device */
        SectorSize = 2048;
    }
    else
    {
        /* This is either a floppy disk device (DrivePartition == 0) or
         * a hard disk device (DrivePartition != 0 && DrivePartition != 0xFF) but
         * it doesn't matter which one because they both have 512 bytes per sector */
        SectorSize = 512;
    }

    if (DrivePartition != 0xff && DrivePartition != 0)
    {
        if (!DiskGetPartitionEntry(DriveNumber, DrivePartition, &PartitionTableEntry))
            return EINVAL;
        SectorOffset = PartitionTableEntry.SectorCountBeforePartition;
        SectorCount = PartitionTableEntry.PartitionSectorCount;
    }

    Context = FrLdrTempAlloc(sizeof(DISKCONTEXT), TAG_HW_DISK_CONTEXT);
    if (!Context)
        return ENOMEM;
    Context->DriveNumber = DriveNumber;
    Context->SectorSize = SectorSize;
    Context->SectorOffset = SectorOffset;
    Context->SectorCount = SectorCount;
    Context->SectorNumber = 0;
    FsSetDeviceSpecific(*FileId, Context);

    return ESUCCESS;
}

static LONG DiskRead(ULONG FileId, VOID* Buffer, ULONG N, ULONG* Count)
{
    DISKCONTEXT* Context = FsGetDeviceSpecific(FileId);
    UCHAR* Ptr = (UCHAR*)Buffer;
    ULONG Length, TotalSectors, MaxSectors, ReadSectors;
    BOOLEAN ret;
    ULONGLONG SectorOffset;

    TotalSectors = (N + Context->SectorSize - 1) / Context->SectorSize;
    MaxSectors   = PcDiskReadBufferSize / Context->SectorSize;
    SectorOffset = Context->SectorNumber + Context->SectorOffset;

    ret = 1;

    while (TotalSectors)
    {
        ReadSectors = TotalSectors;
        if (ReadSectors > MaxSectors)
            ReadSectors = MaxSectors;

        ret = MachDiskReadLogicalSectors(
            Context->DriveNumber,
            SectorOffset,
            ReadSectors,
            (PVOID)DISKREADBUFFER);
        if (!ret)
            break;

        Length = ReadSectors * Context->SectorSize;
        if (Length > N)
            Length = N;

        RtlCopyMemory(Ptr, (PVOID)DISKREADBUFFER, Length);

        Ptr += Length;
        N -= Length;
        SectorOffset += ReadSectors;
        TotalSectors -= ReadSectors;
    }

    *Count = (ULONG)(Ptr - (UCHAR*)Buffer);

    return (!ret) ? EIO : ESUCCESS;
}

static LONG DiskSeek(ULONG FileId, LARGE_INTEGER* Position, SEEKMODE SeekMode)
{
    DISKCONTEXT* Context = FsGetDeviceSpecific(FileId);

    if (SeekMode != SeekAbsolute)
        return EINVAL;
    if (Position->LowPart & (Context->SectorSize - 1))
        return EINVAL;

    Context->SectorNumber = (ULONG)(Position->QuadPart / Context->SectorSize);
    return ESUCCESS;
}

static const DEVVTBL DiskVtbl = {
    DiskClose,
    DiskGetFileInformation,
    DiskOpen,
    DiskRead,
    DiskSeek,
};

PCHAR
GetHarddiskIdentifier(
    UCHAR DriveNumber)
{
    return PcDiskIdentifier[DriveNumber - 0x80];
}

VOID
GetHarddiskInformation(
    UCHAR DriveNumber)
{
    PMASTER_BOOT_RECORD Mbr;
    ULONG *Buffer;
    ULONG i;
    ULONG Checksum;
    ULONG Signature;
    CHAR ArcName[256];
    PARTITION_TABLE_ENTRY PartitionTableEntry;
    PCHAR Identifier = PcDiskIdentifier[DriveNumber - 0x80];

    /* Read the MBR */
    if (!MachDiskReadLogicalSectors(DriveNumber, 0ULL, 1, (PVOID)DISKREADBUFFER))
    {
        ERR("Reading MBR failed\n");
        return;
    }

    Buffer = (ULONG*)DISKREADBUFFER;
    Mbr = (PMASTER_BOOT_RECORD)DISKREADBUFFER;

    Signature =  Mbr->Signature;
    TRACE("Signature: %x\n", Signature);

    /* Calculate the MBR checksum */
    Checksum = 0;
    for (i = 0; i < 128; i++)
    {
        Checksum += Buffer[i];
    }
    Checksum = ~Checksum + 1;
    TRACE("Checksum: %x\n", Checksum);

    /* Fill out the ARC disk block */
    reactos_arc_disk_info[reactos_disk_count].Signature = Signature;
    reactos_arc_disk_info[reactos_disk_count].CheckSum = Checksum;
    sprintf(ArcName, "multi(0)disk(0)rdisk(%lu)", reactos_disk_count);
    strcpy(reactos_arc_strings[reactos_disk_count], ArcName);
    reactos_arc_disk_info[reactos_disk_count].ArcName =
        reactos_arc_strings[reactos_disk_count];
    reactos_disk_count++;

    sprintf(ArcName, "multi(0)disk(0)rdisk(%u)partition(0)", DriveNumber - 0x80);
    FsRegisterDevice(ArcName, &DiskVtbl);

    /* Add partitions */
    i = 1;
    DiskReportError(FALSE);
    while (DiskGetPartitionEntry(DriveNumber, i, &PartitionTableEntry))
    {
        if (PartitionTableEntry.SystemIndicator != PARTITION_ENTRY_UNUSED)
        {
            sprintf(ArcName, "multi(0)disk(0)rdisk(%u)partition(%lu)", DriveNumber - 0x80, i);
            FsRegisterDevice(ArcName, &DiskVtbl);
        }
        i++;
    }
    DiskReportError(TRUE);

    /* Convert checksum and signature to identifier string */
    Identifier[0] = Hex[(Checksum >> 28) & 0x0F];
    Identifier[1] = Hex[(Checksum >> 24) & 0x0F];
    Identifier[2] = Hex[(Checksum >> 20) & 0x0F];
    Identifier[3] = Hex[(Checksum >> 16) & 0x0F];
    Identifier[4] = Hex[(Checksum >> 12) & 0x0F];
    Identifier[5] = Hex[(Checksum >> 8) & 0x0F];
    Identifier[6] = Hex[(Checksum >> 4) & 0x0F];
    Identifier[7] = Hex[Checksum & 0x0F];
    Identifier[8] = '-';
    Identifier[9] = Hex[(Signature >> 28) & 0x0F];
    Identifier[10] = Hex[(Signature >> 24) & 0x0F];
    Identifier[11] = Hex[(Signature >> 20) & 0x0F];
    Identifier[12] = Hex[(Signature >> 16) & 0x0F];
    Identifier[13] = Hex[(Signature >> 12) & 0x0F];
    Identifier[14] = Hex[(Signature >> 8) & 0x0F];
    Identifier[15] = Hex[(Signature >> 4) & 0x0F];
    Identifier[16] = Hex[Signature & 0x0F];
    Identifier[17] = '-';
    Identifier[18] = 'A';
    Identifier[19] = 0;
    TRACE("Identifier: %s\n", Identifier);
}


BOOLEAN
HwInitializeBiosDisks(VOID)
{
    UCHAR DiskCount, DriveNumber;
    ULONG i;
    BOOLEAN Changed;
    CHAR BootPath[512];
    BOOLEAN BootDriveReported = FALSE;

    /* Count the number of visible drives */
    DiskReportError(FALSE);
    DiskCount = 0;
    DriveNumber = 0x80;

    /* There are some really broken BIOSes out there. There are even BIOSes
        * that happily report success when you ask them to read from non-existent
        * harddisks. So, we set the buffer to known contents first, then try to
        * read. If the BIOS reports success but the buffer contents haven't
        * changed then we fail anyway */
    memset((PVOID) DISKREADBUFFER, 0xcd, 512);
    while (MachDiskReadLogicalSectors(DriveNumber, 0ULL, 1, (PVOID)DISKREADBUFFER))
    {
        Changed = FALSE;
        for (i = 0; ! Changed && i < 512; i++)
        {
            Changed = ((PUCHAR)DISKREADBUFFER)[i] != 0xcd;
        }
        if (! Changed)
        {
            TRACE("BIOS reports success for disk %d but data didn't change\n",
                      (int)DiskCount);
            break;
        }

        GetHarddiskInformation(DriveNumber);

        if (FrldrBootDrive == DriveNumber)
            BootDriveReported = TRUE;

        DiskCount++;
        DriveNumber++;
        memset((PVOID) DISKREADBUFFER, 0xcd, 512);
    }
    DiskReportError(TRUE);

    /* Get the drive we're booting from */
    MachDiskGetBootPath(BootPath, sizeof(BootPath));

    /* Add it, if it's a floppy or cdrom */
    if ((FrldrBootDrive >= 0x80 && !BootDriveReported) ||
        DiskIsDriveRemovable(FrldrBootDrive))
    {
        /* TODO: Check if it's really a cdrom drive */
        ULONG* Buffer;
        ULONG Checksum = 0;

        /* Read the MBR */
        if (!MachDiskReadLogicalSectors(FrldrBootDrive, 16ULL, 1, (PVOID)DISKREADBUFFER))
        {
          ERR("Reading MBR failed\n");
          return FALSE;
        }

        Buffer = (ULONG*)DISKREADBUFFER;

        /* Calculate the MBR checksum */
        for (i = 0; i < 2048 / sizeof(ULONG); i++) Checksum += Buffer[i];
        Checksum = ~Checksum + 1;
        TRACE("Checksum: %x\n", Checksum);

        /* Fill out the ARC disk block */
        reactos_arc_disk_info[reactos_disk_count].CheckSum = Checksum;
        strcpy(reactos_arc_strings[reactos_disk_count], BootPath);
        reactos_arc_disk_info[reactos_disk_count].ArcName =
            reactos_arc_strings[reactos_disk_count];
        reactos_disk_count++;

        FsRegisterDevice(BootPath, &DiskVtbl);
        DiskCount++;
    }

    PcBiosDiskCount = DiskCount;
    TRACE("BIOS reports %d harddisk%s\n",
          (int)DiskCount, (DiskCount == 1) ? "": "s");

    return DiskCount != 0;
}
