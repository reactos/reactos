/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/fstub/fstubex.c
* PURPOSE:         Extended FSTUB Routines (not linked to HAL)
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoCreateDisk(IN PDEVICE_OBJECT DeviceObject,
             IN struct _CREATE_DISK* Disk)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoGetBootDiskInformation(IN OUT PBOOTDISK_INFORMATION BootDiskInformation,
                         IN ULONG Size)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoReadDiskSignature(IN PDEVICE_OBJECT DeviceObject,
                    IN ULONG BytesPerSector,
                    OUT PDISK_SIGNATURE Signature)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoReadPartitionTableEx(IN PDEVICE_OBJECT DeviceObject,
                       IN struct _DRIVE_LAYOUT_INFORMATION_EX** DriveLayout)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoSetPartitionInformationEx(IN PDEVICE_OBJECT DeviceObject,
                            IN ULONG PartitionNumber,
                            IN struct _SET_PARTITION_INFORMATION_EX* PartitionInfo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoVerifyPartitionTable(IN PDEVICE_OBJECT DeviceObject,
                       IN BOOLEAN FixErrors)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoWritePartitionTableEx(IN PDEVICE_OBJECT DeviceObject,
                        IN struct _DRIVE_LAYOUT_INFORMATION_EX* DriveLayfout)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
