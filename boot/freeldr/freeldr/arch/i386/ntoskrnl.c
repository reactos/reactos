/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/freeldr/arch/i386/ntoskrnl.c
 * PURPOSE:         NTOS glue routines for the MINIHAL library
 * PROGRAMMERS:     Hervé Poussineau  <hpoussin@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>
#include <ntoskrnl.h>

#ifndef UNIMPLEMENTED
#define UNIMPLEMENTED ASSERT(FALSE)
#endif

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KeInitializeEvent(
    IN PRKEVENT Event,
    IN EVENT_TYPE Type,
    IN BOOLEAN State)
{
    RtlZeroMemory(Event, sizeof(*Event));
}

VOID
NTAPI
KeSetTimeIncrement(
    IN ULONG MaxIncrement,
    IN ULONG MinIncrement)
{
}

VOID
FASTCALL
IoAssignDriveLetters(
    IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
    IN PSTRING NtDeviceName,
    OUT PUCHAR NtSystemPath,
    OUT PSTRING NtSystemPathString)
{
}

NTSTATUS
FASTCALL
IoSetPartitionInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG PartitionNumber,
    IN ULONG PartitionType)
{
    return STATUS_NOT_IMPLEMENTED;
}

#ifndef _M_AMD64
NTSTATUS
NTAPI
IopReadBootRecord(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONGLONG LogicalSectorNumber,
    IN ULONG SectorSize,
    OUT PMASTER_BOOT_RECORD BootRecord)
{
    ULONG_PTR FileId = (ULONG_PTR)DeviceObject;
    LARGE_INTEGER Position;
    ULONG BytesRead;
    ARC_STATUS Status;

    Position.QuadPart = LogicalSectorNumber * SectorSize;
    Status = ArcSeek(FileId, &Position, SeekAbsolute);
    if (Status != ESUCCESS)
        return STATUS_IO_DEVICE_ERROR;

    Status = ArcRead(FileId, BootRecord, SectorSize, &BytesRead);
    if (Status != ESUCCESS || BytesRead != SectorSize)
        return STATUS_IO_DEVICE_ERROR;

    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
IopCopyPartitionRecord(
    IN BOOLEAN ReturnRecognizedPartitions,
    IN ULONG SectorSize,
    IN PPARTITION_TABLE_ENTRY PartitionTableEntry,
    OUT PARTITION_INFORMATION *PartitionEntry)
{
    BOOLEAN IsRecognized = FALSE;

    /* OPTIMIZATION: Fast-path recognition for XP-era compatible filesystems */
    switch (PartitionTableEntry->SystemIndicator)
    {
        case PARTITION_FAT_12:
        case PARTITION_FAT_16:
        case PARTITION_FAT32:
        case PARTITION_IFS: // NTFS/HPFS
        case PARTITION_HUGE:
        case PARTITION_XINT13:
            IsRecognized = TRUE;
            break;
        case PARTITION_ENTRY_UNUSED:
            return FALSE;
        default:
            IsRecognized = FALSE;
            break;
    }

    if (!IsRecognized && ReturnRecognizedPartitions)
        return FALSE;

    PartitionEntry->StartingOffset.QuadPart = (ULONGLONG)PartitionTableEntry->SectorCountBeforePartition * SectorSize;
    PartitionEntry->PartitionLength.QuadPart = (ULONGLONG)PartitionTableEntry->PartitionSectorCount * SectorSize;
    PartitionEntry->HiddenSectors = 0;
    PartitionEntry->PartitionNumber = 0;
    PartitionEntry->PartitionType = PartitionTableEntry->SystemIndicator;
    PartitionEntry->BootIndicator = (PartitionTableEntry->BootIndicator & 0x80) ? TRUE : FALSE;
    PartitionEntry->RecognizedPartition = IsRecognized;
    PartitionEntry->RewritePartition = FALSE;

    return TRUE;
}

NTSTATUS
FASTCALL
IoReadPartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN BOOLEAN ReturnRecognizedPartitions,
    OUT PDRIVE_LAYOUT_INFORMATION *PartitionBuffer)
{
    NTSTATUS Status;
    /* OPTIMIZATION: Use a stack-based buffer instead of ExAllocatePool for the MBR.
       Since we are in the bootloader, 512 bytes on the stack is perfectly safe 
       and avoids the NonPagedPool allocation overhead. */
    UCHAR MbrBuffer[512]; 
    PMASTER_BOOT_RECORD MasterBootRecord = (PMASTER_BOOT_RECORD)MbrBuffer;
    PDRIVE_LAYOUT_INFORMATION Partitions;
    ULONG NbPartitions = 0, i, Size;

    *PartitionBuffer = NULL;

    if (SectorSize < sizeof(MASTER_BOOT_RECORD) || SectorSize > 512)
        return STATUS_NOT_SUPPORTED;

    /* Read disk MBR into stack buffer */
    Status = IopReadBootRecord(DeviceObject, 0, SectorSize, MasterBootRecord);
    if (!NT_SUCCESS(Status))
        return Status;

    if (MasterBootRecord->MasterBootRecordMagic != 0xaa55)
        return STATUS_NOT_SUPPORTED;

    /* OPTIMIZATION: Pre-scan to count active partitions in one pass */
    for (i = 0; i < 4; i++)
    {
        if (MasterBootRecord->PartitionTable[i].SystemIndicator != PARTITION_ENTRY_UNUSED)
            NbPartitions++;
    }

    if (NbPartitions == 0)
        return STATUS_NOT_SUPPORTED;

    /* Allocate storage for the final partition info only once */
    Size = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION, PartitionEntry) +
           NbPartitions * sizeof(PARTITION_INFORMATION);
    Partitions = ExAllocatePool(NonPagedPool, Size);
    if (!Partitions)
        return STATUS_NO_MEMORY;

    /* Reset and fill the partition entries */
    NbPartitions = 0;
    for (i = 0; i < 4; i++)
    {
        if (IopCopyPartitionRecord(ReturnRecognizedPartitions,
                                   SectorSize,
                                   &MasterBootRecord->PartitionTable[i],
                                   &Partitions->PartitionEntry[NbPartitions]))
        {
            Partitions->PartitionEntry[NbPartitions].PartitionNumber = NbPartitions + 1;
            NbPartitions++;
        }
    }

    Partitions->PartitionCount = NbPartitions;
    Partitions->Signature = MasterBootRecord->Signature;

    *PartitionBuffer = Partitions;
    return STATUS_SUCCESS;
}
#endif // _M_AMD64

NTSTATUS
FASTCALL
IoWritePartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfHeads,
    IN PDRIVE_LAYOUT_INFORMATION PartitionBuffer)
{
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
KeStallExecutionProcessor(
    IN ULONG MicroSeconds)
{
    /* OPTIMIZATION: Minimal check before calling the HAL.
       On modern i9, zero-microsecond stalls can still happen due to rounding. */
    if (MicroSeconds == 0) return;
    
    StallExecutionProcessor(MicroSeconds);
}