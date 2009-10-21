/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halamd64/
 * PURPOSE:         I/O HAL Routines for Disk Access
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/




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


