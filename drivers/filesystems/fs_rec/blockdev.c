/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS File System Recognizer
 * FILE:             drivers/filesystems/fs_rec/blockdev.c
 * PURPOSE:          Generic Helper Functions
 * PROGRAMMER:       Alex Ionescu (alex.ionescu@reactos.org)
 *                   Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "fs_rec.h"
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

BOOLEAN
NTAPI
FsRecGetDeviceSectors(IN PDEVICE_OBJECT DeviceObject,
                      IN ULONG SectorSize,
                      OUT PLARGE_INTEGER SectorCount)
{
    PARTITION_INFORMATION PartitionInfo;
    IO_STATUS_BLOCK IoStatusBlock;
    KEVENT Event;
    PIRP Irp;
    NTSTATUS Status;
    ULONG Remainder;
    PAGED_CODE();

    /* Only needed for disks */
    if (DeviceObject->DeviceType != FILE_DEVICE_DISK) return FALSE;

    /* Build the information IRP */
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_PARTITION_INFO,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        &PartitionInfo,
                                        sizeof(PARTITION_INFORMATION),
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp) return FALSE;

    /* Override verification */
    IoGetNextIrpStackLocation(Irp)->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

    /* Do the request */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        /* Wait for completion */
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = IoStatusBlock.Status;
    }

    /* Fail if we couldn't get the data */
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Otherwise, return the number of sectors */
    *SectorCount = RtlExtendedLargeIntegerDivide(PartitionInfo.PartitionLength,
                                                 SectorSize,
                                                 &Remainder);
    return TRUE;
}

BOOLEAN
NTAPI
FsRecGetDeviceSectorSize(IN PDEVICE_OBJECT DeviceObject,
                         OUT PULONG SectorSize)
{
    DISK_GEOMETRY DiskGeometry;
    IO_STATUS_BLOCK IoStatusBlock;
    KEVENT Event;
    PIRP Irp;
    NTSTATUS Status;
    ULONG ControlCode;
    PAGED_CODE();

    /* Check what device we have */
    switch (DeviceObject->DeviceType)
    {
        case FILE_DEVICE_CD_ROM:

            /* Use the CD IOCTL */
            ControlCode = IOCTL_CDROM_GET_DRIVE_GEOMETRY;
            break;

        case FILE_DEVICE_DISK:

            /* Use the Disk IOCTL */
            ControlCode = IOCTL_DISK_GET_DRIVE_GEOMETRY;
            break;

        default:

            /* Invalid device type */
            return FALSE;
    }

    /* Build the information IRP */
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(ControlCode,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        &DiskGeometry,
                                        sizeof(DISK_GEOMETRY),
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp) return FALSE;

    /* Override verification */
    IoGetNextIrpStackLocation(Irp)->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

    /* Do the request */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        /* Wait for completion */
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = IoStatusBlock.Status;
    }

    /* Fail if we couldn't get the data */
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Return the sector size if it's valid */
    if (!DiskGeometry.BytesPerSector) return FALSE;
    *SectorSize = DiskGeometry.BytesPerSector;
    return TRUE;
}

BOOLEAN
NTAPI
FsRecReadBlock(IN PDEVICE_OBJECT DeviceObject,
               IN PLARGE_INTEGER Offset,
               IN ULONG Length,
               IN ULONG SectorSize,
               IN OUT PVOID *Buffer,
               OUT PBOOLEAN DeviceError OPTIONAL)
{
    IO_STATUS_BLOCK IoStatusBlock;
    KEVENT Event;
    PIRP Irp;
    NTSTATUS Status;
    PAGED_CODE();

    /* Assume failure */
    if (DeviceError) *DeviceError = FALSE;

    /* Check if the caller requested too little */
    if (Length < SectorSize)
    {
        /* Read at least the sector size */
        Length = SectorSize;
    }
    else
    {
        /* Otherwise, just round up the request to sector size */
        Length = ROUND_UP(Length, SectorSize);
    }

    /* Check if the caller gave us a buffer */
    if (!*Buffer)
    {
        /* He didn't, allocate one */
        *Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                        PAGE_ROUND_UP(Length),
                                        FSREC_TAG);
        if (!*Buffer) return FALSE;
    }

    /* Build the IRP */
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                       DeviceObject,
                                       *Buffer,
                                       Length,
                                       Offset,
                                       &Event,
                                       &IoStatusBlock);
    if (!Irp) return FALSE;

    /* Override verification */
    IoGetNextIrpStackLocation(Irp)->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

    /* Do the request */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        /* Wait for completion */
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = IoStatusBlock.Status;
    }

    /* Check if we couldn't get the data */
    if (!NT_SUCCESS(Status))
    {
        /* Check if caller wanted to know about the device and fail */
        if (DeviceError) *DeviceError = TRUE;
        return FALSE;
    }

    /* All went well */
    return TRUE;
}

