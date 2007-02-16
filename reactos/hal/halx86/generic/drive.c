/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/drive.c
 * PURPOSE:         I/O HAL Routines for Disk Access
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
HalpAssignDriveLetters(IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
                       IN PSTRING NtDeviceName,
                       OUT PUCHAR NtSystemPath,
                       OUT PSTRING NtSystemPathString)
{
    /* Call the kernel */
    IoAssignDriveLetters(LoaderBlock,
                         NtDeviceName,
                         NtSystemPath,
                         NtSystemPathString);
}

NTSTATUS
NTAPI
HalpReadPartitionTable(IN PDEVICE_OBJECT DeviceObject,
                       IN ULONG SectorSize,
                       IN BOOLEAN ReturnRecognizedPartitions,
                       IN OUT PDRIVE_LAYOUT_INFORMATION *PartitionBuffer)
{
    /* Call the kernel */
    return IoReadPartitionTable(DeviceObject,
                                SectorSize,
                                ReturnRecognizedPartitions,
                                PartitionBuffer);
}

NTSTATUS
NTAPI
HalpWritePartitionTable(IN PDEVICE_OBJECT DeviceObject,
                        IN ULONG SectorSize,
                        IN ULONG SectorsPerTrack,
                        IN ULONG NumberOfHeads,
                        IN PDRIVE_LAYOUT_INFORMATION PartitionBuffer)
{
    /* Call the kernel */
    return IoWritePartitionTable(DeviceObject,
                                 SectorSize,
                                 SectorsPerTrack,
                                 NumberOfHeads,
                                 PartitionBuffer);
}

NTSTATUS
NTAPI
HalpSetPartitionInformation(IN PDEVICE_OBJECT DeviceObject,
                            IN ULONG SectorSize,
                            IN ULONG PartitionNumber,
                            IN ULONG PartitionType)
{
    /* Call the kernel */
    return IoSetPartitionInformation(DeviceObject,
                                     SectorSize,
                                     PartitionNumber,
                                     PartitionType);
}
/* EOF */
