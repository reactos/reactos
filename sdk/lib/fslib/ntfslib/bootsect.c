/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NTFS FS library
 * FILE:        lib/fslib/ntfslib/bootsect.c
 * PURPOSE:     NTFS lib
 * PROGRAMMERS: Pierre Schweitzer, Klachkov Valery
 */

/* INCLUDES ******************************************************************/

#include <ntfslib.h>

#define NDEBUG
#include <debug.h>


/* FUNCTIONS *****************************************************************/

static
VOID
FillJumpInstruction(OUT PBOOT_SECTOR BootSector)
{
    BootSector->Jump[0] = 0xEB;  // jmp
    BootSector->Jump[1] = 0x52;  // 82
    BootSector->Jump[2] = 0x90;  // nop
}

static
VOID
FillOemId(OUT PBOOT_SECTOR BootSector)
{
    BootSector->OEMID.QuadPart = OEM_ID;
}

static
VOID
FillBiosParametersBlock(OUT PBIOS_PARAMETERS_BLOCK BiosParametersBlock)
{
    // See: https://en.wikipedia.org/wiki/BIOS_parameter_block

    BiosParametersBlock->BytesPerSector    = (USHORT)BYTES_PER_SECTOR;
    BiosParametersBlock->SectorsPerCluster = (BYTE)SECTORS_PER_CLUSTER;

    BiosParametersBlock->MediaId = IS_HARD_DRIVE ? 0xF8 : 0xF0;

    BiosParametersBlock->SectorsPerTrack = (USHORT)SECTORS_PER_TRACK;
    BiosParametersBlock->Heads           = DISK_HEADS;
    BiosParametersBlock->HiddenSectorsCount = HIDDEN_SECTORS;
}

static
VOID
FillExBiosParametersBlock(OUT PEXTENDED_BIOS_PARAMETERS_BLOCK ExBiosParametersBlock)
{
    // See: https://en.wikipedia.org/wiki/BIOS_parameter_block

    ExBiosParametersBlock->Header      = EBPB_HEADER;
    ExBiosParametersBlock->SectorCount = TOTAL_SECTORS;

    ExBiosParametersBlock->MftLocation     = LAYOUT.MftLcn;
    ExBiosParametersBlock->MftMirrLocation = LAYOUT.MftMirrLcn;

    ExBiosParametersBlock->ClustersPerMftRecord   = LAYOUT.ClustersPerMftRecord;
    ExBiosParametersBlock->ClustersPerIndexRecord = LAYOUT.ClustersPerIndexRecord;

    ExBiosParametersBlock->SerialNumber = LAYOUT.SerialNumber;
}

//
// Writes the backup boot sector to the last sector of the volume, which is the
// sector immediately following the addressable area (LAYOUT.TotalSectors).
//
static
NTSTATUS
WriteBackupBootSector(IN PBOOT_SECTOR BootSector)
{
    NTSTATUS        Status;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER   Offset;
    PBYTE           Sector;
    ULONG           Bps = BYTES_PER_SECTOR;

    Sector = RtlAllocateHeap(RtlGetProcessHeap(), 0, Bps);
    if (!Sector)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(Sector, Bps);
    RtlCopyMemory(Sector, BootSector, (Bps < sizeof(BOOT_SECTOR)) ? Bps : sizeof(BOOT_SECTOR));

    Offset.QuadPart = (LONGLONG)TOTAL_SECTORS * Bps;

    Status = NtWriteFile(DISK_HANDLE,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         Sector,
                         Bps,
                         &Offset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Backup boot sector write failed (Status %lx)\n", Status);
    }

    FREE(Sector);

    return Status;
}

NTSTATUS
WriteBootSector(VOID)
{
    NTSTATUS        Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PBOOT_SECTOR    BootSector;
    PBYTE           BootRegion;
    LARGE_INTEGER   Offset;
    ULONG           RegionSize;

    // The whole $Boot region (8192 bytes rounded up to clusters). Only the
    // first sector carries the BPB; the rest is zeroed bootstrap area.
    RegionSize = LAYOUT.BootClusters * BYTES_PER_CLUSTER;

    BootRegion = RtlAllocateHeap(RtlGetProcessHeap(), 0, RegionSize);
    if (!BootRegion)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(BootRegion, RegionSize);

    BootSector = (PBOOT_SECTOR)BootRegion;

    FillJumpInstruction(BootSector);
    FillOemId(BootSector);

    FillBiosParametersBlock(&(BootSector->BPB));
    FillExBiosParametersBlock(&(BootSector->EBPB));

    BootSector->EndSector = BOOT_SECTOR_END;

    // Write the $Boot region at LCN 0.
    Offset.QuadPart = 0LL;
    Status = NtWriteFile(DISK_HANDLE,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         BootRegion,
                         RegionSize,
                         &Offset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("BootSector write failed. NtWriteFile() failed (Status %lx)\n", Status);
        FREE(BootRegion);
        return Status;
    }

    // Write the backup boot sector at the very end of the volume.
    Status = WriteBackupBootSector(BootSector);

    FREE(BootRegion);

    return Status;
}
