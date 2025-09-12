/*
 * PROJECT:     ReactOS InPort (Bus) Mouse Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     I/O control handling
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include "inport.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
InPortInternalDeviceControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    PCONNECT_DATA ConnectData;
    PINPORT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("%s(%p, %p) 0x%X\n", __FUNCTION__, DeviceObject, Irp,
           IrpSp->Parameters.DeviceIoControl.IoControlCode);

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_INTERNAL_MOUSE_CONNECT:
            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(CONNECT_DATA))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            /* Already connected */
            if (DeviceExtension->ClassService)
            {
                Status = STATUS_SHARING_VIOLATION;
                break;
            }

            ConnectData = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

            DeviceExtension->ClassDeviceObject = ConnectData->ClassDeviceObject;
            DeviceExtension->ClassService = ConnectData->ClassService;

            Status = STATUS_SUCCESS;
            break;

        case IOCTL_INTERNAL_MOUSE_DISCONNECT:
            DeviceExtension->ClassService = NULL;

            Status = STATUS_SUCCESS;
            break;

        case IOCTL_MOUSE_QUERY_ATTRIBUTES:
            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(MOUSE_ATTRIBUTES))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            *(PMOUSE_ATTRIBUTES)Irp->AssociatedIrp.SystemBuffer = DeviceExtension->MouseAttributes;
            Irp->IoStatus.Information = sizeof(MOUSE_ATTRIBUTES);

            Status = STATUS_SUCCESS;
            break;

        default:
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}
