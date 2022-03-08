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

/*
 * NTSTATUS
 * FASTCALL
 * IoReadPartitionTable(
 *     IN PDEVICE_OBJECT DeviceObject,
 *     IN ULONG SectorSize,
 *     IN BOOLEAN ReturnRecognizedPartitions,
 *     OUT PDRIVE_LAYOUT_INFORMATION *PartitionBuffer);
 *
 * See boot/freeldr/freeldr/disk/partition.c
 */

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
