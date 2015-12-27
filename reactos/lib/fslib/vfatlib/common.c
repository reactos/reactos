/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS VFAT filesystem library
 * FILE:        lib\fslib\vfatlib\common.c
 * PURPOSE:     Common code for Fat support
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Eric Kohl
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

/***** Wipe function for FAT12 and FAT16 formats, adapted from FAT32 code *****/
NTSTATUS
Fat1216WipeSectors(
    IN HANDLE FileHandle,
    IN PFAT16_BOOT_SECTOR BootSector,
    IN OUT PFORMAT_CONTEXT Context)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PUCHAR Buffer;
    LARGE_INTEGER FileOffset;
    ULONGLONG Sector;
    ULONG SectorsHuge;
    ULONG Length;
    NTSTATUS Status;

    /* Allocate buffer for the cluster */
    Buffer = (PUCHAR)RtlAllocateHeap(RtlGetProcessHeap(),
                                     HEAP_ZERO_MEMORY,
                                     BootSector->SectorsPerCluster * BootSector->BytesPerSector);
    if (Buffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Sector = 0;
    Length = BootSector->SectorsPerCluster * BootSector->BytesPerSector;

    SectorsHuge = (BootSector->SectorsHuge != 0 ? BootSector->SectorsHuge : BootSector->Sectors);

    while (Sector + BootSector->SectorsPerCluster < SectorsHuge)
    {
        FileOffset.QuadPart = Sector * BootSector->BytesPerSector;

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

        UpdateProgress(Context, (ULONG)BootSector->SectorsPerCluster);

        Sector += BootSector->SectorsPerCluster;
    }

    if (Sector + BootSector->SectorsPerCluster > SectorsHuge)
    {
        DPRINT("Remaining sectors %lu\n", SectorsHuge - Sector);

        FileOffset.QuadPart = Sector * BootSector->BytesPerSector;
        Length = (SectorsHuge - Sector) * BootSector->BytesPerSector;

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

        UpdateProgress(Context, SectorsHuge - Sector);
    }

done:
    /* Free the buffer */
    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
    return Status;
}

/* EOF */
