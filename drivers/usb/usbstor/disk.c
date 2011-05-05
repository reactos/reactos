/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Storage Driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbstor/disk.c
 * PURPOSE:     USB block storage device driver.
 * PROGRAMMERS:
 *              James Tabor
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbstor.h"

NTSTATUS
USBSTOR_HandleExecuteSCSI(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OUT PSCSI_REQUEST_BLOCK Request,
    IN PPDO_DEVICE_EXTENSION PDODeviceExtension)
{
    DPRINT1("USBSTOR_HandleExecuteSCSI\n");

    DbgBreakPoint();

    Request->SrbStatus = SRB_STATUS_ERROR;
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
USBSTOR_HandleInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PSCSI_REQUEST_BLOCK Request;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    NTSTATUS Status;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get request block
    //
    Request = (PSCSI_REQUEST_BLOCK)IoStack->Parameters.Others.Argument1;

    //
    // get device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(Request);
    ASSERT(PDODeviceExtension);

    switch(Request->Function)
    {
        case SRB_FUNCTION_EXECUTE_SCSI:
        {
            DPRINT1("SRB_FUNCTION_EXECUTE_SCSI\n");
            Status = USBSTOR_HandleExecuteSCSI(DeviceObject, Irp, Request, PDODeviceExtension);

        }
        case SRB_FUNCTION_RELEASE_DEVICE:
        {
            DPRINT1("SRB_FUNCTION_RELEASE_DEVICE\n");
            //
            // sanity check
            //
            ASSERT(PDODeviceExtension->Claimed == TRUE);

            //
            // release claim
            //
            PDODeviceExtension->Claimed = TRUE;
            Status = STATUS_SUCCESS;
            break;
        }
        case SRB_FUNCTION_CLAIM_DEVICE:
        {
            DPRINT1("SRB_FUNCTION_CLAIM_DEVICE\n");
            //
            // check if the device has been claimed
            //
            if (PDODeviceExtension->Claimed)
            {
                //
                // device has already been claimed
                //
                Status = STATUS_DEVICE_BUSY;
                Request->SrbStatus = SRB_STATUS_BUSY;
                break;
            }

            //
            // claim device
            //
            PDODeviceExtension->Claimed = TRUE;

            //
            // output device object
            //
            Request->DataBuffer = DeviceObject;

            //
            // completed successfully
            //
            Status = STATUS_SUCCESS;
            break;
        }
        case SRB_FUNCTION_RELEASE_QUEUE:
        {
            DPRINT1("SRB_FUNCTION_RELEASE_QUEUE UNIMPLEMENTED\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
        case SRB_FUNCTION_FLUSH:
        {
            DPRINT1("SRB_FUNCTION_FLUSH UNIMPLEMENTED\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
        case SRB_FUNCTION_SET_LINK_TIMEOUT:
        {
            DPRINT1("SRB_FUNCTION_FLUSH UNIMPLEMENTED\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
        default:
        {
            //
            // not supported
            //
            Status = STATUS_NOT_SUPPORTED;
            Request->SrbStatus = SRB_STATUS_ERROR;
        }
    }

    return Status;
}

NTSTATUS
USBSTOR_HandleDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT1("USBSTOR_HandleDeviceControl IoControl %x\n", IoStack->Parameters.DeviceIoControl.IoControlCode);
    DPRINT1("USBSTOR_HandleDeviceControl InputBufferLength %x\n", IoStack->Parameters.DeviceIoControl.InputBufferLength);
    DPRINT1("USBSTOR_HandleDeviceControl OutputBufferLength %x\n", IoStack->Parameters.DeviceIoControl.OutputBufferLength);
    DPRINT1("USBSTOR_HandleDeviceControl InputBuffer %x\n", IoStack->Parameters.DeviceIoControl.Type3InputBuffer);
    DPRINT1("USBSTOR_HandleDeviceControl SystemBuffer %x\n", Irp->AssociatedIrp.SystemBuffer);
    DPRINT1("USBSTOR_HandleDeviceControl UserBuffer %x\n", Irp->UserBuffer);
    DPRINT1("USBSTOR_HandleDeviceControl MdlAddress %x\n", Irp->MdlAddress);

    //IOCTL_STORAGE_QUERY_PROPERTY

    return STATUS_NOT_SUPPORTED;
}
