/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/legacy/wdmaud/deviface.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Andrew Greenwood
 *                  Johannes Anderwald
 */
#include "wdmaud.h"


NTSTATUS
NTAPI
WdmAudDeviceControl(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(WDMAUD_DEVICE_INFO))
    {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_PARAMETER;
    }

    DPRINT1("WdmAudDeviceControl entered\n");

    switch(IoStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_OPEN_WDMAUD:
            //return WdmAudDeviceControlOpen(DeviceObject, Irp);
        case IOCTL_CLOSE_WDMAUD:
        case IOCTL_GETNUMDEVS_TYPE:
        case IOCTL_SETDEVICE_STATE:
        case IOCTL_GETDEVID:
        case IOCTL_GETVOLUME:
        case IOCTL_SETVOLUME:
        case IOCTL_GETCAPABILITIES:
        case IOCTL_WRITEDATA:
           break;
    }




    UNIMPLEMENTED

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}
