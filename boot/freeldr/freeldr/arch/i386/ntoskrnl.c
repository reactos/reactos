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
    BOOLEAN IsRecognized;

    IsRecognized = TRUE; /* FIXME */
    if (!IsRecognized && ReturnRecognizedPartitions)
        return FALSE;

    PartitionEntry->StartingOffset.QuadPart = (ULONGLONG)PartitionTableEntry->SectorCountBeforePartition * SectorSize;
    PartitionEntry->PartitionLength.QuadPart = (ULONGLONG)PartitionTableEntry->PartitionSectorCount * SectorSize;
    PartitionEntry->HiddenSectors = 0;
    PartitionEntry->PartitionNumber = 0; /* Will be filled later */
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
    PMASTER_BOOT_RECORD MasterBootRecord;
    PDRIVE_LAYOUT_INFORMATION Partitions;
    ULONG NbPartitions, i, Size;

    *PartitionBuffer = NULL;

    if (SectorSize < sizeof(MASTER_BOOT_RECORD))
        return STATUS_NOT_SUPPORTED;

    MasterBootRecord = ExAllocatePool(NonPagedPool, SectorSize);
    if (!MasterBootRecord)
        return STATUS_NO_MEMORY;

    /* Read disk MBR */
    Status = IopReadBootRecord(DeviceObject, 0, SectorSize, MasterBootRecord);
    if (!NT_SUCCESS(Status))
    {
        ExFreePool(MasterBootRecord);
        return Status;
    }

    /* Check validity of boot record */
    if (MasterBootRecord->MasterBootRecordMagic != 0xaa55)
    {
        ExFreePool(MasterBootRecord);
        return STATUS_NOT_SUPPORTED;
    }

    /* Count number of partitions */
    NbPartitions = 0;
    for (i = 0; i < 4; i++)
    {
        NbPartitions++;

        if (MasterBootRecord->PartitionTable[i].SystemIndicator == PARTITION_EXTENDED ||
            MasterBootRecord->PartitionTable[i].SystemIndicator == PARTITION_XINT13_EXTENDED)
        {
            /* FIXME: unhandled case; count number of partitions */
            UNIMPLEMENTED;
        }
    }

    if (NbPartitions == 0)
    {
        ExFreePool(MasterBootRecord);
        return STATUS_NOT_SUPPORTED;
    }

    /* Allocation space to store partitions */
    Size = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION, PartitionEntry) +
           NbPartitions * sizeof(PARTITION_INFORMATION);
    Partitions = ExAllocatePool(NonPagedPool, Size);
    if (!Partitions)
    {
        ExFreePool(MasterBootRecord);
        return STATUS_NO_MEMORY;
    }

    /* Count number of partitions */
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

        if (MasterBootRecord->PartitionTable[i].SystemIndicator == PARTITION_EXTENDED ||
            MasterBootRecord->PartitionTable[i].SystemIndicator == PARTITION_XINT13_EXTENDED)
        {
            /* FIXME: unhandled case; copy partitions */
            UNIMPLEMENTED;
        }
    }

    Partitions->PartitionCount = NbPartitions;
    Partitions->Signature = MasterBootRecord->Signature;
    ExFreePool(MasterBootRecord);

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
    StallExecutionProcessor(MicroSeconds);
}
