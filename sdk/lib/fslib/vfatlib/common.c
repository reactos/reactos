/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS VFAT filesystem library
 * FILE:        lib\fslib\vfatlib\common.c
 * PURPOSE:     Common code for Fat support
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Eric Kohl
 *              Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "vfatlib.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

ULONG
GetShiftCount(IN ULONG Value)
{
    ULONG i = 1;

    while (Value > 0)
    {
        i++;
        Value /= 2;
    }

    return i - 2;
}

ULONG
CalcVolumeSerialNumber(VOID)
{
    LARGE_INTEGER SystemTime;
    TIME_FIELDS TimeFields;
    ULONG Serial;
    PUCHAR Buffer;

    NtQuerySystemTime(&SystemTime);
    RtlTimeToTimeFields(&SystemTime, &TimeFields);

    Buffer = (PUCHAR)&Serial;
    Buffer[0] = (UCHAR)(TimeFields.Year & 0xFF) + (UCHAR)(TimeFields.Hour & 0xFF);
    Buffer[1] = (UCHAR)(TimeFields.Year >> 8) + (UCHAR)(TimeFields.Minute & 0xFF);
    Buffer[2] = (UCHAR)(TimeFields.Month & 0xFF) + (UCHAR)(TimeFields.Second & 0xFF);
    Buffer[3] = (UCHAR)(TimeFields.Day & 0xFF) + (UCHAR)(TimeFields.Milliseconds & 0xFF);

    return Serial;
}

/***** Wipe function for FAT12, FAT16 and FAT32 formats *****/
NTSTATUS
FatWipeSectors(
    IN HANDLE FileHandle,
    IN ULONG TotalSectors,
    IN ULONG SectorsPerCluster,
    IN ULONG BytesPerSector,
    IN OUT PFORMAT_CONTEXT Context)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PUCHAR Buffer;
    LARGE_INTEGER FileOffset;
    ULONGLONG Sector;
    ULONG Length;
    NTSTATUS Status;

    Length = SectorsPerCluster * BytesPerSector;

    /* Allocate buffer for the cluster */
    Buffer = (PUCHAR)RtlAllocateHeap(RtlGetProcessHeap(),
                                     HEAP_ZERO_MEMORY,
                                     Length);
    if (Buffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Wipe all clusters */
    Sector = 0;
    while (Sector + SectorsPerCluster < TotalSectors)
    {
        FileOffset.QuadPart = Sector * BytesPerSector;

        Status = NtWriteFile(FileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             Buffer,
                             Length,
                             &FileOffset,
                             NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
            goto done;
        }

        UpdateProgress(Context, SectorsPerCluster);

        Sector += SectorsPerCluster;
    }

    /* Wipe the trailing space behind the last cluster */
    if (Sector < TotalSectors)
    {
        DPRINT("Remaining sectors %lu\n", TotalSectors - Sector);

        FileOffset.QuadPart = Sector * BytesPerSector;
        Length = (TotalSectors - Sector) * BytesPerSector;

        Status = NtWriteFile(FileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             Buffer,
                             Length,
                             &FileOffset,
                             NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
            goto done;
        }

        UpdateProgress(Context, TotalSectors - Sector);
    }

done:
    /* Free the buffer */
    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
    return Status;
}

/* EOF */
