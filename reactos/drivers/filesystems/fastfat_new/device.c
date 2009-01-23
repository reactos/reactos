/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/device.c
 * PURPOSE:         Device control
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
FatDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DPRINT1("FatDeviceControl()\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
FatPerformDevIoCtrl(PDEVICE_OBJECT DeviceObject,
                    ULONG ControlCode,
                    PVOID InputBuffer,
                    ULONG InputBufferSize,
                    PVOID OutputBuffer,
                    ULONG OutputBufferSize,
                    BOOLEAN Override)
{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack;
    IO_STATUS_BLOCK IoStatus;

    /* Initialize the event for waiting */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* Build the device I/O control request */
    Irp = IoBuildDeviceIoControlRequest(ControlCode,
                                        DeviceObject,
                                        InputBuffer,
                                        InputBufferSize,
                                        OutputBuffer,
                                        OutputBufferSize,
                                        FALSE,
                                        &Event,
                                        &IoStatus);

    /* Fail if IRP hasn't been allocated */
    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    /* Set verify override flag if requested */
    if (Override)
    {
        Stack = IoGetNextIrpStackLocation(Irp);
        Stack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
    }

    /* Call the driver */
    Status = IoCallDriver(DeviceObject, Irp);

    /* Wait if needed */
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    return Status;
}

/* EOF */
