/**
 * cbi.c - USB driver stack project for Windows NT 4.0
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

VOID umss_cbi_send_adsc_complete(PURB purb, PVOID context);
VOID umss_cbi_transfer_data(PUMSS_DEVICE_EXTENSION pdev_ext);
VOID umss_cbi_get_status(PUMSS_DEVICE_EXTENSION pdev_ext);
VOID umss_cbi_transfer_data_complete(PURB purb, PVOID context);
VOID umss_cbi_get_status_complete(PURB purb, PVOID context);

NTSTATUS
umss_class_specific_request(IN PUMSS_DEVICE_EXTENSION pdev_ext,
                            IN UCHAR request,
                            IN UCHAR dir,
                            IN PVOID buffer,
                            IN ULONG buffer_length,
                            IN PURBCOMPLETION completion)
{
    PURB purb;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(dir);

    purb = usb_alloc_mem(NonPagedPool, sizeof(URB));
    // Build URB for the ADSC command
    UsbBuildVendorRequest(purb,
                          pdev_ext->dev_handle | 0xffff,
                          buffer,
                          buffer_length,
                          0x21, request, 0, pdev_ext->pif_desc->bInterfaceNumber, completion, pdev_ext, 0);

    status = usb_submit_urb(pdev_ext->dev_mgr, purb);
    if (status != STATUS_PENDING)
    {
        usb_free_mem(purb);
        purb = NULL;
        return status;
    }
    dev_mgr_register_irp(pdev_ext->dev_mgr, pdev_ext->io_packet.pirp, purb);
    return status;
}

NTSTATUS
umss_cbi_startio(IN PUMSS_DEVICE_EXTENSION pdev_ext, IN PIO_PACKET io_packet)
{
    NTSTATUS status;

    status = STATUS_NOT_SUPPORTED;
    return status;

    RtlCopyMemory(&pdev_ext->io_packet, io_packet, sizeof(pdev_ext->io_packet));

    // Send the ADSC request to the device
    // Calls UMSS_CbiSendADSCComplete when transfer completes
    status = umss_class_specific_request(pdev_ext,
                                         ACCEPT_DEVICE_SPECIFIC_COMMAND,
                                         USB_DIR_OUT,
                                         io_packet->cdb, io_packet->cdb_length, umss_cbi_send_adsc_complete);

    return status;
}



VOID
umss_cbi_send_adsc_complete(PURB purb, PVOID context)
{
    NTSTATUS status;
    PUMSS_DEVICE_EXTENSION pdev_ext;
    PIO_PACKET io_packet;

    pdev_ext = (PUMSS_DEVICE_EXTENSION) context;
    io_packet = &pdev_ext->io_packet;

    status = purb->status;

    dev_mgr_remove_irp(pdev_ext->dev_mgr, pdev_ext->io_packet.pirp);

    if (!usb_success(status))
    {
        usb_dbg_print(DBGLVL_MINIMUM, ("umss_cbi_send_adsc_complete(): Command Block Failure!!!\n"));

        // BUGBUG - Should reset device here?
        // Device failed Command Block, complete with error
        umss_complete_request(pdev_ext, STATUS_IO_DEVICE_ERROR);

    }
    else if (io_packet->data_length)
    {

        usb_dbg_print(DBGLVL_HIGH, ("umss_cbi_send_adsc_complete(): Queuing Data Transfer DPC\n"));
        umss_cbi_transfer_data(pdev_ext);

    }
    else if (pdev_ext->pif_desc->bInterfaceProtocol == PROTOCOL_CBI)
    {
        // Device supports interrupt pipe, so get status
        umss_cbi_get_status(pdev_ext);
    }
    else
    {
        // Device does not report status, so complete request
        umss_complete_request(pdev_ext, STATUS_SUCCESS);
    }

    usb_free_mem(purb);
    purb = NULL;
}



VOID
umss_cbi_reset_pipe(IN PVOID reference)
{
    PUMSS_DEVICE_EXTENSION pdev_ext;
    pdev_ext = (PUMSS_DEVICE_EXTENSION) reference;

    // Reset the appropriate pipe, based on data direction
    umss_reset_pipe(pdev_ext,
                    (pdev_ext->io_packet.flags & USB_DIR_IN) ?
                    usb_make_handle((pdev_ext->dev_handle >> 16), pdev_ext->if_idx, pdev_ext->in_endp_idx) :
                    usb_make_handle((pdev_ext->dev_handle >> 16), pdev_ext->if_idx, pdev_ext->out_endp_idx));

    // Device stalled endpoint, so complete I/O operation with error.
    // BUGBUG is this correct?  Check spec...
    umss_complete_request(pdev_ext, USB_STATUS_STALL_PID);
}

VOID
umss_cbi_transfer_data(PUMSS_DEVICE_EXTENSION pdev_ext)
{
    PVOID buffer = NULL;
    ULONG buffer_length;

    // Get next data buffer element, if any.
    buffer = umss_get_buffer(pdev_ext, &buffer_length);
    if (NULL == buffer)
    {
        //Done with data phase, so move to status phase if (supported)

        if (pdev_ext->pif_desc->bInterfaceProtocol == PROTOCOL_CBI)
        {
            // Device supports interrupt pipe, so get status
            umss_cbi_get_status(pdev_ext);
        }
        else
        {
            // No interrupt pipe, so just complete the request
            umss_complete_request(pdev_ext, STATUS_SUCCESS);
        }
    }
    else
    {
        // Transfer next element of the data phase
        umss_bulk_transfer(pdev_ext,
                           (UCHAR) ((pdev_ext->io_packet.flags & USB_DIR_IN) ? USB_DIR_IN : USB_DIR_OUT),
                           buffer, buffer_length, umss_cbi_transfer_data_complete);
    }
}


VOID
umss_cbi_transfer_data_complete(PURB purb, PVOID context)
{
    NTSTATUS status;
    PUMSS_DEVICE_EXTENSION pdev_ext;

    pdev_ext = (PUMSS_DEVICE_EXTENSION) context;
    status = purb->status;

    usb_free_mem(purb);
    purb = NULL;

    if (!usb_success(status))
    {
        // Device failed Data Transfer
        // Check if we need to clear stalled pipe
        if (usb_halted(status))
        {
            // Reset pipe can only be done at passive level, so we need
            // to schedule a work item to do it.
            if (!umss_schedule_workitem
                ((PVOID) pdev_ext, umss_cbi_reset_pipe, pdev_ext->dev_mgr, pdev_ext->dev_handle))
            {
                usb_dbg_print(DBGLVL_MINIMUM,
                              ("umss_cbi_transfer_data_complete(): Failed to allocate work-item to reset pipe!\n"));
                TRAP();
                umss_complete_request(pdev_ext, STATUS_IO_DEVICE_ERROR);
            }
        }
        else
        {
            umss_complete_request(pdev_ext, STATUS_IO_DEVICE_ERROR);
        }
        return;
    }
    // Transfer succeeded
    // umss_cbi_transfer_data( pdev_ext );
    umss_complete_request(pdev_ext, STATUS_SUCCESS);
    return;
}


VOID
umss_cbi_get_status(PUMSS_DEVICE_EXTENSION pdev_ext)
{
    PURB purb;
    NTSTATUS status;

    purb = usb_alloc_mem(NonPagedPool, sizeof(URB));
    if (purb == NULL)
        return;

    // Build a URB for our interrupt transfer
    UsbBuildInterruptOrBulkTransferRequest(purb,
                                           usb_make_handle((pdev_ext->dev_handle >> 16), pdev_ext->if_idx,
                                                           pdev_ext->int_endp_idx), (PUCHAR) & pdev_ext->idb,
                                           sizeof(INTERRUPT_DATA_BLOCK), umss_cbi_get_status_complete,
                                           pdev_ext, 0);

    // Call USB driver stack
    status = usb_submit_urb(pdev_ext->dev_mgr, purb);
    if (status != STATUS_PENDING)
    {
        usb_free_mem(purb);
        purb = NULL;
        return;
    }
    dev_mgr_register_irp(pdev_ext->dev_mgr, pdev_ext->io_packet.pirp, purb);
    return;
}


VOID
umss_cbi_get_status_complete(PURB purb, PVOID context)
{
    NTSTATUS status;
    PUMSS_DEVICE_EXTENSION pdev_ext;
    PINTERRUPT_DATA_BLOCK idb;

    pdev_ext = (PUMSS_DEVICE_EXTENSION) context;

    status = purb->status;
    dev_mgr_remove_irp(pdev_ext->dev_mgr, pdev_ext->io_packet.pirp);

    usb_free_mem(purb);
    purb = NULL;

    if (!usb_success(status))
    {
        // Device failed Data Transfer
        // Check if we need to clear stalled pipe
        if (usb_halted(status))
        {
            if (!umss_schedule_workitem
                ((PVOID) pdev_ext, umss_cbi_reset_pipe, pdev_ext->dev_mgr, pdev_ext->dev_handle))
            {
                usb_dbg_print(DBGLVL_MINIMUM,
                              ("umss_cbi_get_status_complete(): Failed to allocate work-item to reset pipe!\n"));
                TRAP();
                umss_complete_request(pdev_ext, STATUS_IO_DEVICE_ERROR);
                return;
            }
        }
        umss_complete_request(pdev_ext, STATUS_IO_DEVICE_ERROR);
        return;
    }

    // Interrupt transfer succeeded
    idb = &(pdev_ext->idb);

    // Check for an error in the status block
    if ((0 != idb->bType) || (0 != (idb->bValue & 0x3)))
    {
        umss_complete_request(pdev_ext, STATUS_IO_DEVICE_ERROR);
    }
    else
    {
        umss_complete_request(pdev_ext, STATUS_SUCCESS);
    }
}
