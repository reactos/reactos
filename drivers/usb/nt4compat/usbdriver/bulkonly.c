/**
 * bulkonly.c - USB driver stack project for Windows NT 4.0
 *
 * Copyright (c) 2002-2004 Zhiming  mypublic99@yahoo.com
 *
 * This program/include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program/include file is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program (in the main directory of the distribution, the file
 * COPYING); if not, write to the Free Software Foundation,Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "usbdriver.h"
#include <ntddscsi.h>

#define OLYMPUS_CSW( pdev_EXT, staTUS ) \
( ( ( pdev_EXT )->flags & UMSS_DEV_FLAG_OLYMPUS_DEV ) ? ( ( staTUS ) == CSW_OLYMPUS_SIGNATURE ) : FALSE )

BOOLEAN umss_clear_pass_through_length(PIO_PACKET io_packet);

NTSTATUS umss_bulkonly_send_sense_req(PUMSS_DEVICE_EXTENSION pdev_ext);

VOID umss_bulkonly_send_cbw_completion(IN PURB purb, IN PVOID context);

VOID umss_bulkonly_transfer_data(PUMSS_DEVICE_EXTENSION pdev_ext);

VOID umss_sync_submit_urb_completion(PURB purb, PVOID context);

NTSTATUS umss_sync_submit_urb(PUMSS_DEVICE_EXTENSION pdev_ext, PURB purb);

VOID umss_bulkonly_get_status(PUMSS_DEVICE_EXTENSION pdev_ext);

VOID umss_bulkonly_transfer_data_complete(PURB purb, PVOID reference);

VOID umss_bulkonly_reset_pipe_and_get_status(IN PVOID reference);

VOID umss_bulkonly_reset_recovery(IN PVOID reference);

VOID umss_bulkonly_get_status_complete(IN PURB purb, IN PVOID context);

/*++
Routine Description:

    Handler for all I/O requests using bulk-only protocol.

 Arguments:

    DeviceExtension - Device extension for our FDO.

Return Value:

    NONE

--*/
NTSTATUS
umss_bulkonly_startio(IN PUMSS_DEVICE_EXTENSION pdev_ext, IN PIO_PACKET io_packet)
{
    PCOMMAND_BLOCK_WRAPPER cbw;
    NTSTATUS status;

    if (pdev_ext == NULL || io_packet == NULL || io_packet->pirp == NULL)
        return STATUS_INVALID_PARAMETER;

    pdev_ext->retry = TRUE;
    RtlCopyMemory(&pdev_ext->io_packet, io_packet, sizeof(pdev_ext->io_packet));

    // Setup the command block wrapper for this request
    cbw = &pdev_ext->cbw;
    cbw->dCBWSignature = CBW_SIGNATURE;
    cbw->dCBWTag = 0;
    cbw->dCBWDataTransferLength = io_packet->data_length;
    cbw->bmCBWFlags = (io_packet->flags & USB_DIR_IN) ? 0x80 : 0;
    cbw->bCBWLun = io_packet->lun;
    cbw->bCBWLength = io_packet->cdb_length;
    RtlCopyMemory(cbw->CBWCB, io_packet->cdb, sizeof(cbw->CBWCB));

    RtlZeroMemory(&pdev_ext->csw, sizeof(pdev_ext->csw));
    // Send the command block wrapper to the device.
    // Calls UMSS_BulkOnlySendCBWComplete when transfer completes.
    status = umss_bulk_transfer(pdev_ext,
                                USB_DIR_OUT,
                                cbw, sizeof(COMMAND_BLOCK_WRAPPER), umss_bulkonly_send_cbw_completion);

    return status;
}


NTSTATUS
umss_bulk_transfer(IN PUMSS_DEVICE_EXTENSION pdev_ext,
                   IN UCHAR trans_dir, IN PVOID buf, IN ULONG buf_length, IN PURBCOMPLETION completion)
{
    PURB purb;
    NTSTATUS status;
    DEV_HANDLE endp_handle;

    if (pdev_ext == NULL || buf == NULL || completion == NULL)
        return STATUS_INVALID_PARAMETER;

    if (buf_length > (ULONG) MAX_BULK_TRANSFER_LENGTH)
        return STATUS_INVALID_PARAMETER;

    purb = usb_alloc_mem(NonPagedPool, sizeof(URB));

    if (purb == NULL)
        return STATUS_NO_MEMORY;

    if (trans_dir == USB_DIR_OUT)
    {
        endp_handle = usb_make_handle((pdev_ext->dev_handle >> 16), pdev_ext->if_idx, pdev_ext->out_endp_idx);
    }
    else
    {
        endp_handle = usb_make_handle((pdev_ext->dev_handle >> 16), pdev_ext->if_idx, pdev_ext->in_endp_idx);
    }

    UsbBuildInterruptOrBulkTransferRequest(purb, endp_handle, buf, buf_length, completion, pdev_ext, 0);
    dev_mgr_register_irp(pdev_ext->dev_mgr, pdev_ext->io_packet.pirp, purb);
    status = usb_submit_urb(pdev_ext->dev_mgr, purb);
    if (status == STATUS_PENDING)
    {
        return status;
    }

    dev_mgr_remove_irp(pdev_ext->dev_mgr, pdev_ext->io_packet.pirp);
    if (purb)
    {
        usb_free_mem(purb);
        purb = NULL;
    }
    return status;
}

VOID
umss_bulkonly_send_cbw_completion(IN PURB purb, IN PVOID context)
{
    NTSTATUS status;
    PUMSS_DEVICE_EXTENSION pdev_ext;

    pdev_ext = (PUMSS_DEVICE_EXTENSION) context;

    status = purb->status;
    dev_mgr_remove_irp(pdev_ext->dev_mgr, pdev_ext->io_packet.pirp);

    if ((pdev_ext->io_packet.flags & IOP_FLAG_STAGE_MASK) == IOP_FLAG_STAGE_SENSE)
    {
        usb_free_mem(purb->data_buffer);
        purb->data_buffer = NULL;
        purb->data_length = 0;
    }

    if (status != STATUS_SUCCESS)
    {
        if (usb_halted(status))
        {
            //Schedule a work-item to do a reset recovery
            if (!umss_schedule_workitem
                ((PVOID) pdev_ext, umss_bulkonly_reset_recovery, pdev_ext->dev_mgr, pdev_ext->dev_handle))
            {
                umss_complete_request(pdev_ext, STATUS_IO_DEVICE_ERROR);
            }
        }
        else
        {
            // Device failed CBW without stalling, so complete with error
            umss_complete_request(pdev_ext, status);
        }
    }
    else
    {
        // CBW was accepted by device, so start data phase of I/O operation
        umss_bulkonly_transfer_data(pdev_ext);
    }

    usb_free_mem(purb);

    purb = NULL;
    return;
}

//can only be called at passive level
NTSTATUS
umss_sync_submit_urb(PUMSS_DEVICE_EXTENSION pdev_ext, PURB purb)
{
    NTSTATUS status;

    if (pdev_ext == NULL || purb == NULL)
        return STATUS_INVALID_PARAMETER;

    purb->completion = umss_sync_submit_urb_completion;
    purb->context = (PVOID) pdev_ext;

    dev_mgr_register_irp(pdev_ext->dev_mgr, pdev_ext->io_packet.pirp, purb);
    status = usb_submit_urb(pdev_ext->dev_mgr, purb);
    if (status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&pdev_ext->sync_event, Executive, KernelMode, TRUE, NULL);
        status = purb->status;
    }
    else
        dev_mgr_remove_irp(pdev_ext->dev_mgr, pdev_ext->io_packet.pirp);

    return status;
}

VOID
umss_sync_submit_urb_completion(PURB purb, PVOID context)
{
    PUMSS_DEVICE_EXTENSION pdev_ext;

    if (purb == NULL || context == NULL)
        return;

    pdev_ext = (PUMSS_DEVICE_EXTENSION) context;
    dev_mgr_remove_irp(pdev_ext->dev_mgr, pdev_ext->io_packet.pirp);
    KeSetEvent(&pdev_ext->sync_event, 0, FALSE);
    return;
}

/*++
Routine Description:

    Worker function used to execute a reset recovery after a stall.

 Arguments:

    Reference - Our device extension.

Return Value:

    NONE

--*/
VOID
umss_bulkonly_reset_recovery(IN PVOID reference)
{
    PUMSS_DEVICE_EXTENSION pdev_ext;
    URB urb;
    NTSTATUS status;
    DEV_HANDLE endp_handle;

    pdev_ext = (PUMSS_DEVICE_EXTENSION) reference;
    usb_dbg_print(DBGLVL_MAXIMUM, ("umss_bulkonly_reset_recovery(): entering...\n"));
    // Steps for reset recovery:
    // 1. Send device a mass storage reset command on the default endpoint.
    // 2. Reset the bulk-in endpoint.
    // 3. Reset the bulk-out endpoint.
    // 4. Complete the original I/O request with error.


    // Build the mass storage reset command
    UsbBuildVendorRequest(&urb, pdev_ext->dev_handle | 0xffff,  //default pipe
                          NULL, //no extra data
                          0,    //no size
                          0x21, //class, interface
                          BULK_ONLY_MASS_STORAGE_RESET,
                          0,
                          pdev_ext->pif_desc->bInterfaceNumber,
                          NULL, //completion
                          NULL, //context
                          0);   //reference

    // Send mass storage reset command to device
    status = umss_sync_submit_urb(pdev_ext, &urb);

    if (status != STATUS_SUCCESS)
    {
        usb_dbg_print(DBGLVL_MINIMUM, ("umss_bulkonly_reset_recovery(): Reset Recovery failed!\n"));
    }
    else
    {
        //Reset Bulk-in endpoint
        endp_handle = usb_make_handle((pdev_ext->dev_handle >> 16), pdev_ext->if_idx, pdev_ext->in_endp_idx);
        status = umss_reset_pipe(pdev_ext, endp_handle);

        if (!NT_SUCCESS(status))
        {
            usb_dbg_print(DBGLVL_MINIMUM,
                          ("umss_bulkonly_reset_recovery(): Unable to clear Bulk-in endpoint\n"));
        }

        //Reset Bulk-out endpoint
        endp_handle = usb_make_handle((pdev_ext->dev_handle >> 16), pdev_ext->if_idx, pdev_ext->out_endp_idx);
        status = umss_reset_pipe(pdev_ext, endp_handle);

        if (!NT_SUCCESS(status))
        {
            usb_dbg_print(DBGLVL_MINIMUM,
                          ("umss_bulkonly_reset_recovery(): Unable to clear Bulk-out endpoint\n"));
        }
    }
    umss_complete_request(pdev_ext, status);
}

/*++
Routine Description:

    Schedules a bulk data transfer to/from the device.

 Arguments:

    DeviceExtension - Our FDO's device extension.

Return Value:

    NONE

--*/
VOID
umss_bulkonly_transfer_data(PUMSS_DEVICE_EXTENSION pdev_ext)
{
    PVOID data_buf;
    ULONG data_buf_length;
    NTSTATUS status;
    UCHAR trans_dir = USB_DIR_IN; // FIXME: Initialize this properly!

    // Steps for data phase
    // 1. Get data buffer fragment (either SGD list, flat buffer, or none).
    // 2. Schedule data transfer if neccessary.
    // 3. Repeat 1-2 until all data transferred, or endpoint stalls.
    // 4. Move to status phase.

    // Get next data buffer element, if any
    data_buf = umss_get_buffer(pdev_ext, &data_buf_length);

    if (NULL == data_buf)
    {
        //No data to transfer, so move to status phase
        umss_bulkonly_get_status(pdev_ext);
    }
    else
    {
        // Schedule the data transfer.
        // Calls umss_bulkonly_transfer_data_complete when transfer completes.

        if ((pdev_ext->io_packet.flags & IOP_FLAG_STAGE_MASK) == IOP_FLAG_STAGE_NORMAL)
            trans_dir = (UCHAR) ((pdev_ext->cbw.bmCBWFlags & USB_DIR_IN) ? USB_DIR_IN : USB_DIR_OUT);
        else if ((pdev_ext->io_packet.flags & IOP_FLAG_STAGE_MASK) == IOP_FLAG_STAGE_SENSE)
            trans_dir = USB_DIR_IN;

        if ((status = umss_bulk_transfer(pdev_ext,
                                         trans_dir,
                                         data_buf,
                                         data_buf_length,
                                         umss_bulkonly_transfer_data_complete)) != STATUS_PENDING)
        {
            umss_complete_request(pdev_ext, status);
        }
    }
    return;
}

/*++
Routine Description:
    Completion handler for bulk data transfer requests.
--*/
VOID
umss_bulkonly_transfer_data_complete(PURB purb, PVOID reference)
{
    NTSTATUS status;
    PUMSS_DEVICE_EXTENSION pdev_ext;
    pdev_ext = (PUMSS_DEVICE_EXTENSION) reference;

    status = purb->status;

    dev_mgr_remove_irp(pdev_ext->dev_mgr, pdev_ext->io_packet.pirp);

    if (status != STATUS_SUCCESS)
    {
        //
        // clear the data length if this is a scsi pass through request
        //
        umss_clear_pass_through_length(&pdev_ext->io_packet);

        // Device failed data phase
        // Check if we need to clear stalled pipe
        if (usb_halted(status))
        {
            PULONG buf;
            buf = usb_alloc_mem(NonPagedPool, 32);
            buf[0] = (ULONG) pdev_ext;
            buf[1] = (ULONG) purb->endp_handle;

            usb_dbg_print(DBGLVL_MINIMUM, ("umss_transfer_data_complete(): transfer data error!\n"));
            if (!umss_schedule_workitem
                ((PVOID) buf, umss_bulkonly_reset_pipe_and_get_status, pdev_ext->dev_mgr,
                 pdev_ext->dev_handle))
            {
                usb_free_mem(buf), buf = NULL;
                usb_dbg_print(DBGLVL_MINIMUM,
                              ("umss_transfer_data_complete(): Failed to allocate work-item to reset pipe!\n"));
                TRAP();
                umss_complete_request(pdev_ext, status);
            }
        }
        else
        {
            //finish our request
            umss_complete_request(pdev_ext, status);
        }
    }
    else
    {
        // Start next part of data phase
        //umss_bulkonly_transfer_data( pdev_ext );
        umss_bulkonly_get_status(pdev_ext);
        //umss_complete_request( pdev_ext, status );
    }

    usb_free_mem(purb);
    purb = NULL;

    return;                     // STATUS_MORE_PROCESSING_REQUIRED;
}


VOID
umss_bulkonly_reset_pipe_and_get_status(IN PVOID reference)
{
    PUMSS_DEVICE_EXTENSION pdev_ext;
    DEV_HANDLE endp_handle;
    NTSTATUS status;

    usb_dbg_print(DBGLVL_MINIMUM, ("umss_bulkonly_reset_pipe_and_get_status(): entering...\n"));

    pdev_ext = (PUMSS_DEVICE_EXTENSION) (((PULONG) reference)[0]);
    endp_handle = (DEV_HANDLE) ((PULONG) reference)[1];
    usb_free_mem(reference);
    reference = NULL;

    // Reset the endpoint
    if ((status = umss_reset_pipe(pdev_ext, endp_handle)) != STATUS_SUCCESS)
    {
        usb_dbg_print(DBGLVL_MINIMUM, ("umss_bulkonly_reset_pipe_and_get_status(): reset pipe failed\n"));
        umss_complete_request(pdev_ext, status);
        return;
    }
    // Data phase is finished since the endpoint stalled, so go to status phase
    usb_dbg_print(DBGLVL_MINIMUM,
                  ("umss_bulkonly_reset_pipe_and_get_status(): reset pipe succeeds, continue to get status\n"));
    umss_bulkonly_get_status(pdev_ext);
}

VOID
umss_bulkonly_get_status(PUMSS_DEVICE_EXTENSION pdev_ext)
{
    NTSTATUS status;
    // Schedule bulk transfer to get command status wrapper from device
    status = umss_bulk_transfer(pdev_ext,
                                USB_DIR_IN,
                                &(pdev_ext->csw),
                                sizeof(COMMAND_STATUS_WRAPPER), umss_bulkonly_get_status_complete);
    if (status != STATUS_PENDING)
    {
        umss_complete_request(pdev_ext, status);
    }
}

/*++
Routine Description:

    Completion handler for bulk data transfer request.

 Arguments:

    DeviceObject - Previous device object.
    Irp - Irp used for sending command.
    Reference - Our FDO.

Return Value:

    Driver-originated IRPs always return STATUS_MORE_PROCESSING_REQUIRED.

--*/
VOID
umss_bulkonly_get_status_complete(IN PURB purb, IN PVOID context)
{
    NTSTATUS status;
    PUMSS_DEVICE_EXTENSION pdev_ext;
    PCOMMAND_STATUS_WRAPPER csw;

    pdev_ext = (PUMSS_DEVICE_EXTENSION) context;
    status = purb->status;

    dev_mgr_remove_irp(pdev_ext->dev_mgr, pdev_ext->io_packet.pirp);

    csw = &(pdev_ext->csw);
    if (status == STATUS_SUCCESS &&
        ((csw->dCSWSignature == CSW_SIGNATURE) || OLYMPUS_CSW(pdev_ext, csw->dCSWSignature)))
    {
        if (csw->bCSWStatus == CSW_STATUS_PASSED)
        {
            // Received valid CSW with good status

            if ((pdev_ext->io_packet.flags & IOP_FLAG_STAGE_MASK) == IOP_FLAG_STAGE_NORMAL &&
                (pdev_ext->io_packet.flags & IOP_FLAG_REQ_SENSE) && pdev_ext->io_packet.sense_data != NULL)
                UMSS_FORGE_GOOD_SENSE(pdev_ext->io_packet.sense_data)
                    umss_complete_request(pdev_ext, STATUS_SUCCESS);
        }
        else if (csw->bCSWStatus == CSW_STATUS_FAILED)
        {
            // start a request sense if necessary
            if ((pdev_ext->io_packet.flags & IOP_FLAG_REQ_SENSE) &&
                (pdev_ext->io_packet.flags & IOP_FLAG_STAGE_MASK) == IOP_FLAG_STAGE_NORMAL)
            {
                if (umss_bulkonly_send_sense_req(pdev_ext) != STATUS_PENDING)
                {
                    // don't know how to handle.
                    umss_complete_request(pdev_ext, STATUS_IO_DEVICE_ERROR);
                }
                else
                {
                    // fall through to free the urb
                }
            }
            else
            {
                // error occurred, reset device
                if (!umss_schedule_workitem
                    ((PVOID) pdev_ext, umss_bulkonly_reset_recovery, pdev_ext->dev_mgr, pdev_ext->dev_handle))
                {
                    umss_complete_request(pdev_ext, STATUS_IO_DEVICE_ERROR);
                }
            }
        }
        else
        {
            // error occurred, reset device
            if (!umss_schedule_workitem
                ((PVOID) pdev_ext, umss_bulkonly_reset_recovery, pdev_ext->dev_mgr, pdev_ext->dev_handle))
            {
                umss_complete_request(pdev_ext, STATUS_IO_DEVICE_ERROR);
            }
        }
    }
    else if ((status != STATUS_SUCCESS) && (usb_halted(status)) && (pdev_ext->retry))
    {
        // Device stalled CSW transfer, retry once before failing
        PULONG buf;
        pdev_ext->retry = FALSE;

        buf = usb_alloc_mem(NonPagedPool, 32);
        buf[0] = (ULONG) pdev_ext;
        buf[1] = (ULONG) purb->endp_handle;

        if (!umss_schedule_workitem
            ((PVOID) buf, umss_bulkonly_reset_pipe_and_get_status, pdev_ext->dev_mgr, pdev_ext->dev_handle))
        {
            usb_free_mem(buf), buf = NULL;
            usb_dbg_print(DBGLVL_MINIMUM,
                          ("umss_bulkonly_get_status_complete(): Failed to allocate work-item to reset pipe!\n"));
            TRAP();
            umss_complete_request(pdev_ext, status);
        }
    }
    else if (status != STATUS_CANCELLED)
    {
        // An error has occured.  Reset the device.
        if (!umss_schedule_workitem
            ((PVOID) pdev_ext, umss_bulkonly_reset_recovery, pdev_ext->dev_mgr, pdev_ext->dev_handle))
        {
            usb_dbg_print(DBGLVL_MINIMUM,
                          ("umss_bulkonly_get_status_complete(): Failed to schedule work-item to reset pipe!\n"));
            TRAP();
            umss_complete_request(pdev_ext, status);
        }
    }
    else
    {
        // the request is canceled
        usb_dbg_print(DBGLVL_MINIMUM, ("umss_bulkonly_get_status_complete(): the request is canceled\n"));
        umss_complete_request(pdev_ext, STATUS_CANCELLED);
    }

    usb_free_mem(purb);
    purb = NULL;

    return;
}

/*++
Routine Description:

    Queries Bulk-Only device for maximum LUN number

 Arguments:

    DeviceExtension - Our device extension.

Return Value:

    Maximum LUN number for device, or 0 if error occurred.

--*/
CHAR
umss_bulkonly_get_maxlun(IN PUMSS_DEVICE_EXTENSION pdev_ext)
{
    PURB purb = NULL;
    UCHAR max_lun;
    NTSTATUS status;

    purb = usb_alloc_mem(NonPagedPool, sizeof(URB));

    if (!purb)
    {
        usb_dbg_print(DBGLVL_MINIMUM,
                      ("umss_bulkonly_get_maxlun(): Failed to allocate URB, setting max LUN to 0\n"));
        max_lun = 0;
    }
    else
    {
        // Build the get max lun command
        UsbBuildVendorRequest(purb, (pdev_ext->dev_handle | 0xffff), &max_lun, sizeof(max_lun), 0xb1,   //class, interface, in
                              BULK_ONLY_GET_MAX_LUN, 0, pdev_ext->pif_desc->bInterfaceNumber, NULL, NULL, 0);

        // Send get max lun command to device
        status = umss_sync_submit_urb(pdev_ext, purb);

        if (status != STATUS_PENDING)
        {
            usb_dbg_print(DBGLVL_MINIMUM,
                          ("umss_bulkonly_get_maxlun(): Get Max LUN command failed, setting max LUN to 0!\n"));
            max_lun = 0;
        }
    }

    if (purb)
        usb_free_mem(purb);

    usb_dbg_print(DBGLVL_MINIMUM, ("umss_bulkonly_get_maxlun(): Max LUN = %x\n", max_lun));

    return max_lun;
}

PVOID
umss_get_buffer(PUMSS_DEVICE_EXTENSION pdev_ext, ULONG * buf_length)
{
    PVOID buffer;

    if ((pdev_ext->io_packet.flags & IOP_FLAG_STAGE_MASK) == IOP_FLAG_STAGE_NORMAL)
    {
        buffer = (PVOID) pdev_ext->io_packet.data_buffer;
        *buf_length = pdev_ext->io_packet.data_length;
    }
    else if ((pdev_ext->io_packet.flags & IOP_FLAG_STAGE_MASK) == IOP_FLAG_STAGE_SENSE)
    {
        buffer = (PVOID) pdev_ext->io_packet.sense_data;
        *buf_length = pdev_ext->io_packet.sense_data_length;
    }
    else
    {
        buffer = NULL;
        *buf_length = 0;
    }

    return buffer;
}

BOOLEAN
umss_bulkonly_build_sense_cdb(PUMSS_DEVICE_EXTENSION pdev_ext, PCOMMAND_BLOCK_WRAPPER cbw)
{
    UCHAR sub_class;
    PUCHAR cdb;

    if (pdev_ext == NULL || cbw == NULL)
        return FALSE;

    cdb = cbw->CBWCB;
    RtlZeroMemory(cdb, MAX_CDB_LENGTH);
    sub_class = pdev_ext->pif_desc->bInterfaceSubClass;

    cdb[0] = SFF_REQUEST_SENSEE;
    cdb[1] = pdev_ext->io_packet.lun << 5;
    cdb[4] = 18;

    switch (sub_class)
    {
    case UMSS_SUBCLASS_SFF8070I:
    case UMSS_SUBCLASS_UFI:
        {
            cbw->bCBWLength = 12;
            break;
        }
    case UMSS_SUBCLASS_RBC:
    case UMSS_SUBCLASS_SCSI_TCS:
        {
            cbw->bCBWLength = 6;
            break;
        }
    default:
        return FALSE;
    }
    return TRUE;
}

NTSTATUS
umss_bulkonly_send_sense_req(PUMSS_DEVICE_EXTENSION pdev_ext)
{
    PCOMMAND_BLOCK_WRAPPER cbw;
    NTSTATUS status;

    if (pdev_ext == NULL || pdev_ext->io_packet.sense_data == NULL
        || pdev_ext->io_packet.sense_data_length < 18)
        return STATUS_INVALID_PARAMETER;

    pdev_ext->retry = TRUE;

    cbw = usb_alloc_mem(NonPagedPool, sizeof(COMMAND_BLOCK_WRAPPER));
    RtlZeroMemory(cbw, sizeof(COMMAND_BLOCK_WRAPPER));
    pdev_ext->io_packet.flags &= ~IOP_FLAG_STAGE_MASK;
    pdev_ext->io_packet.flags |= IOP_FLAG_STAGE_SENSE;

    cbw->dCBWSignature = CBW_SIGNATURE;
    cbw->dCBWTag = 0;
    cbw->dCBWDataTransferLength = pdev_ext->io_packet.sense_data_length;
    cbw->bmCBWFlags = USB_DIR_IN;
    cbw->bCBWLun = 0;

    if (umss_bulkonly_build_sense_cdb(pdev_ext, cbw) == FALSE)
    {
        usb_free_mem(cbw);
        cbw = NULL;
        return STATUS_UNSUCCESSFUL;
    }

    status = umss_bulk_transfer(pdev_ext,
                                USB_DIR_OUT,
                                cbw, sizeof(COMMAND_BLOCK_WRAPPER), umss_bulkonly_send_cbw_completion);

    if (status != STATUS_PENDING)
    {
        usb_free_mem(cbw);
        cbw = NULL;
    }
    return status;
}

BOOLEAN
umss_clear_pass_through_length(PIO_PACKET io_packet)
{
    //
    // clear the respective data length to meet request of scsi pass through requirement.
    //

    BOOLEAN sense_stage;
    ULONG ctrl_code;
    PIO_STACK_LOCATION cur_stack;
    PSCSI_PASS_THROUGH pass_through;
    PSCSI_PASS_THROUGH_DIRECT pass_through_direct;

    if (io_packet == NULL)
        return FALSE;

    if ((io_packet->flags & IOP_FLAG_SCSI_CTRL_TRANSFER) == 0)
        return FALSE;

    sense_stage = FALSE;
    if (io_packet->flags & IOP_FLAG_STAGE_SENSE)
        sense_stage = TRUE;

    cur_stack = IoGetCurrentIrpStackLocation(io_packet->pirp);
    ctrl_code = cur_stack->Parameters.DeviceIoControl.IoControlCode;
    if (ctrl_code == IOCTL_SCSI_PASS_THROUGH_DIRECT)
    {
        pass_through_direct = io_packet->pirp->AssociatedIrp.SystemBuffer;
        if (sense_stage)
            pass_through_direct->SenseInfoLength = 0;
        else
            pass_through_direct->DataTransferLength = 0;
    }
    else if (ctrl_code == IOCTL_SCSI_PASS_THROUGH)
    {
        pass_through = io_packet->pirp->AssociatedIrp.SystemBuffer;
        if (sense_stage)
            pass_through->SenseInfoLength = 0;
        else
            pass_through->DataTransferLength = 0;
    }
    else
        return FALSE;

    return TRUE;
}
