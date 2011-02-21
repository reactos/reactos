/**
 * dmgrdisp.c - USB driver stack project for Windows NT 4.0
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

VOID
disp_urb_completion(PURB purb, PVOID context)
{
    PUSB_DEV_MANAGER dev_mgr;
    ULONG ctrl_code;
    NTSTATUS status;
    PDEVEXT_HEADER dev_hdr;

    UNREFERENCED_PARAMETER(context);

    if (purb == NULL)
        return;

    ctrl_code = (ULONG) purb->reference;
    dev_mgr = (PUSB_DEV_MANAGER) purb->context;

    // at this stage, the irp can not be canceled since the urb
    // won't be found in any queue and the irp is not in any queue.
    // see line 4685 in hub.c
    // Sometimes, it may be very fast to enter this routine before
    // the dev_mgr_register_irp to be called in dispatch routine in
    // usb2.0 environment as
    // we did in usb1.1 driver. We can not simply add a loop to wait
    // for the dispatch thread to add the irp to the list, because
    // here we are at DPC level higher than the dispatch thread
    // running level. And the solution is to register the irp
    // before the urb is scheduled instead of registering it after
    // urb is scheduled.
    if (purb->pirp)
    {
        PIO_STACK_LOCATION irp_stack;
        dev_mgr_remove_irp(dev_mgr, purb->pirp);

        status = purb->status;
        irp_stack = IoGetCurrentIrpStackLocation(purb->pirp);

        if (purb->status != STATUS_SUCCESS)
        {
            purb->pirp->IoStatus.Information = 0;
        }
        else
        {
            // currently only IRP_MJ_DEVICE_CONTROL and IRP_MJ_INTERNAL_DEVICE_CONTROL
            // are allowed. And we do not need to set information
            // for IRP_MJ_INTERNAL_DEVICE_CONTROL
            if (irp_stack->MajorFunction == IRP_MJ_DEVICE_CONTROL)
                purb->pirp->IoStatus.Information = purb->data_length;
        }
        purb->pirp->IoStatus.Status = status;
        if (irp_stack)
        {
            dev_hdr = irp_stack->DeviceObject->DeviceExtension;
            if (dev_hdr->start_io)
            {
                IoStartNextPacket(irp_stack->DeviceObject, TRUE);
            }
        }
        IoCompleteRequest(purb->pirp, IO_NO_INCREMENT);
    }
    return;
}

VOID
disp_noio_urb_completion(PURB purb, PVOID context)
{
    PUSB_CTRL_SETUP_PACKET psetup;
    PURB purb2;
    PUSB_DEV_MANAGER dev_mgr;
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irp_stack;
    PDEVEXT_HEADER dev_hdr;

    if (purb == NULL)
        return;

    psetup = (PUSB_CTRL_SETUP_PACKET) purb->setup_packet;

    if ((psetup->bmRequestType == 0x2) &&
        (psetup->bRequest == USB_REQ_CLEAR_FEATURE) &&
        (psetup->wIndex == 0))       //reset pipe
    {
        purb2 = (PURB) context;
    }
    else
    {
        purb2 = purb;
    }

    if (purb2->pirp == NULL)
        return;

    dev_mgr = (PUSB_DEV_MANAGER) purb2->context;

    dev_mgr_remove_irp(dev_mgr, purb2->pirp);

    if (purb->status != STATUS_SUCCESS)
        status = STATUS_IO_DEVICE_ERROR;

    purb2->pirp->IoStatus.Information = 0;
    purb2->pirp->IoStatus.Status = status;
    irp_stack = IoGetCurrentIrpStackLocation(purb->pirp);
    if (irp_stack)
    {
        dev_hdr = irp_stack->DeviceObject->DeviceExtension;
        if (dev_hdr->start_io)
        {
            IoStartNextPacket(irp_stack->DeviceObject, TRUE);
        }
    }
    IoCompleteRequest(purb2->pirp, IO_NO_INCREMENT);
    return;
}

//this function is called by the hcd's
//dispatch when they have done their job.
NTSTATUS
dev_mgr_dispatch(IN PUSB_DEV_MANAGER dev_mgr, IN PIRP irp)
{
    PIO_STACK_LOCATION irp_stack;
    NTSTATUS status;
    ULONG ctrl_code;
    USE_NON_PENDING_IRQL;

    ASSERT(irp);
    if (dev_mgr == NULL)
    {
        EXIT_DISPATCH(STATUS_INVALID_PARAMETER, irp);
    }

    status = STATUS_SUCCESS;
    irp_stack = IoGetCurrentIrpStackLocation(irp);
    ctrl_code = irp_stack->Parameters.DeviceIoControl.IoControlCode;

    switch (irp_stack->MajorFunction)
    {
    case IRP_MJ_CREATE:
        {
            InterlockedIncrement(&dev_mgr->open_count);
            EXIT_DISPATCH(STATUS_SUCCESS, irp);
        }
    case IRP_MJ_CLOSE:
        {
            InterlockedDecrement(&dev_mgr->open_count);
            EXIT_DISPATCH(STATUS_SUCCESS, irp);
        }
    case IRP_MJ_INTERNAL_DEVICE_CONTROL:
    case IRP_MJ_DEVICE_CONTROL:
        {
            switch (ctrl_code)
            {
            case IOCTL_GET_DEV_COUNT:
                {
                    LONG dev_count;

                    irp->IoStatus.Information = 0;
                    if (irp_stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(LONG))
                    {
                        EXIT_DISPATCH(STATUS_INVALID_PARAMETER, irp);
                    }

                    KeAcquireSpinLock(&dev_mgr->dev_list_lock, &old_irql);
                    dev_count = usb_count_list(&dev_mgr->dev_list);
                    KeReleaseSpinLock(&dev_mgr->dev_list_lock, old_irql);

                    *((PLONG) irp->AssociatedIrp.SystemBuffer) = dev_count;
                    irp->IoStatus.Information = sizeof(LONG);
                    EXIT_DISPATCH(STATUS_SUCCESS, irp);
                }
            case IOCTL_ENUM_DEVICES:
                {
                    PLIST_ENTRY pthis, pnext;
                    LONG dev_count, array_size, i, j = 0;
                    PUSB_DEV pdev;
                    PENUM_DEV_ARRAY peda;

                    irp->IoStatus.Information = 0;
                    if (irp_stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(LONG))
                    {
                        EXIT_DISPATCH(STATUS_INVALID_PARAMETER, irp);
                    }
                    if (irp_stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ENUM_DEV_ARRAY))
                    {
                        EXIT_DISPATCH(STATUS_INVALID_PARAMETER, irp);
                    }
                    array_size = *((PULONG) irp->AssociatedIrp.SystemBuffer);

                    KeAcquireSpinLock(&dev_mgr->dev_list_lock, &old_irql);
                    dev_count = usb_count_list(&dev_mgr->dev_list);
                    dev_count = dev_count > array_size ? array_size : dev_count;
                    peda = (PENUM_DEV_ARRAY) irp->AssociatedIrp.SystemBuffer;
                    RtlZeroMemory(peda, sizeof(ENUM_DEV_ARRAY) + (dev_count - 1) * sizeof(ENUM_DEV_ELEMENT));

                    if (dev_count)
                    {
                        ListFirst(&dev_mgr->dev_list, pthis);
                        for(i = 0, j = 0; i < dev_count; i++)
                        {
                            pdev = struct_ptr(pthis, USB_DEV, dev_link);
                            ListNext(&dev_mgr->dev_list, pthis, pnext);
                            pthis = pnext;

                            lock_dev(pdev, FALSE);
                            if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
                            {
                                unlock_dev(pdev, FALSE);
                                continue;
                            }

                            if (dev_state(pdev) < USB_DEV_STATE_ADDRESSED)
                            {
                                unlock_dev(pdev, FALSE);
                                continue;
                            }

                            peda->dev_arr[i].dev_handle = (pdev->dev_id << 16);
                            //may not get the desc yet
                            if (pdev->pusb_dev_desc)
                            {
                                peda->dev_arr[i].product_id = pdev->pusb_dev_desc->idProduct;
                                peda->dev_arr[i].vendor_id = pdev->pusb_dev_desc->idVendor;
                            }
                            else
                            {
                                peda->dev_arr[i].product_id = 0xffff;
                                peda->dev_arr[i].vendor_id = 0xffff;
                            }
                            peda->dev_arr[i].dev_addr = pdev->dev_addr;
                            unlock_dev(pdev, FALSE);
                            j++;
                        }
                    }
                    peda->dev_count = dev_count ? j : 0;
                    KeReleaseSpinLock(&dev_mgr->dev_list_lock, old_irql);

                    irp->IoStatus.Information =
                        sizeof(ENUM_DEV_ARRAY) + (dev_count - 1) * sizeof(ENUM_DEV_ELEMENT);
                    EXIT_DISPATCH(STATUS_SUCCESS, irp);
                }
            case IOCTL_GET_DEV_DESC:
                {
                    GET_DEV_DESC_REQ gddr;
                    PUSB_DESC_HEADER pusb_desc_header;
                    PUSB_DEV pdev;
                    LONG buf_size;

                    if (irp_stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(GET_DEV_DESC_REQ))
                    {
                        EXIT_DISPATCH(STATUS_INVALID_PARAMETER, irp);
                    }

                    if (irp_stack->Parameters.DeviceIoControl.OutputBufferLength < 8)
                    {
                        EXIT_DISPATCH(STATUS_INVALID_PARAMETER, irp);
                    }

                    status = STATUS_SUCCESS;
                    buf_size = irp_stack->Parameters.DeviceIoControl.OutputBufferLength;
                    RtlCopyMemory(&gddr, irp->AssociatedIrp.SystemBuffer, sizeof(GET_DEV_DESC_REQ));
                    pusb_desc_header = irp->AssociatedIrp.SystemBuffer;

                    if (gddr.desc_type != USB_DT_CONFIG && gddr.desc_type != USB_DT_DEVICE)
                    {
                        EXIT_DISPATCH(STATUS_INVALID_DEVICE_REQUEST, irp);
                    }

                    if (usb_query_and_lock_dev(dev_mgr, gddr.dev_handle, &pdev) != STATUS_SUCCESS)
                    {
                        EXIT_DISPATCH(STATUS_IO_DEVICE_ERROR, irp);
                    }

                    lock_dev(pdev, FALSE);
                    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
                    {
                        status = STATUS_INVALID_DEVICE_STATE;
                        goto ERROR_OUT;
                    }
                    if (dev_state(pdev) != USB_DEV_STATE_ADDRESSED &&
                        dev_state(pdev) != USB_DEV_STATE_CONFIGURED)
                    {
                        status = STATUS_DEVICE_NOT_READY;
                        goto ERROR_OUT;
                    }

                    if (pdev->pusb_dev_desc == NULL)
                    {
                        status = STATUS_DEVICE_NOT_READY;
                        goto ERROR_OUT;
                    }

                    if (gddr.desc_type == USB_DT_DEVICE)
                    {
                        RtlCopyMemory(pusb_desc_header,
                                      pdev->pusb_dev_desc,
                                      buf_size > sizeof(USB_DEVICE_DESC)
                                      ? sizeof(USB_DEVICE_DESC) : buf_size);

                        irp->IoStatus.Information =
                            buf_size >= sizeof(USB_DEVICE_DESC) ? sizeof(USB_DEVICE_DESC) : buf_size;
                    }
                    else if (gddr.desc_type == USB_DT_CONFIG)
                    {
                        PUSB_CONFIGURATION_DESC pusb_config_desc;
                        if (pdev->pusb_dev_desc->bNumConfigurations <= gddr.desc_idx)
                        {
                            status = STATUS_INVALID_PARAMETER;
                            goto ERROR_OUT;
                        }

                        pusb_config_desc = usb_find_config_desc_by_idx((PUCHAR) & pdev->pusb_dev_desc[1],
                                                                       gddr.desc_idx,
                                                                       pdev->pusb_dev_desc->
                                                                       bNumConfigurations);

                        if (pusb_config_desc == NULL)
                        {
                            status = STATUS_DEVICE_NOT_READY;
                            goto ERROR_OUT;
                        }

                        RtlCopyMemory(pusb_desc_header,
                                      pusb_config_desc,
                                      buf_size >= pusb_config_desc->wTotalLength
                                      ? pusb_config_desc->wTotalLength : buf_size);

                        irp->IoStatus.Information =
                            buf_size >= pusb_config_desc->wTotalLength
                            ? pusb_config_desc->wTotalLength : buf_size;
                    }
                  ERROR_OUT:
                    unlock_dev(pdev, FALSE);
                    usb_unlock_dev(pdev);
                    EXIT_DISPATCH(status, irp);
                }
            case IOCTL_SUBMIT_URB_RD:
            case IOCTL_SUBMIT_URB_WR:
            case IOCTL_SUBMIT_URB_NOIO:
                {
                    PURB purb;
                    ULONG endp_idx, if_idx, user_buffer_length = 0;
                    PUCHAR user_buffer = NULL;
                    PUSB_DEV pdev;
                    DEV_HANDLE endp_handle;
                    PUSB_ENDPOINT pendp;

                    if (irp_stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(URB))
                    {
                        EXIT_DISPATCH(STATUS_INVALID_PARAMETER, irp);
                    }

                    purb = (PURB) irp->AssociatedIrp.SystemBuffer;
                    endp_handle = purb->endp_handle;

                    if (ctrl_code == IOCTL_SUBMIT_URB_RD || ctrl_code == IOCTL_SUBMIT_URB_WR)
                    {
                        if (irp_stack->MajorFunction == IRP_MJ_DEVICE_CONTROL)
                        {
                            user_buffer_length = irp_stack->Parameters.DeviceIoControl.OutputBufferLength;
                            if (user_buffer_length == 0)
                                EXIT_DISPATCH(STATUS_INVALID_PARAMETER, irp);
                            user_buffer = MmGetSystemAddressForMdl(irp->MdlAddress);
                        }
                        else
                        {
                            if (purb->data_buffer == NULL || purb->data_length == 0)
                                EXIT_DISPATCH(STATUS_INVALID_PARAMETER, irp);
                            user_buffer_length = purb->data_length;
                            user_buffer = purb->data_buffer;
                        }
                    }

                    if (usb_query_and_lock_dev(dev_mgr, endp_handle & ~0xffff, &pdev) != STATUS_SUCCESS)
                    {
                        EXIT_DISPATCH(STATUS_IO_DEVICE_ERROR, irp);
                    }


                    lock_dev(pdev, FALSE);
                    if (dev_state(pdev) == USB_DEV_STATE_ZOMB || (dev_state(pdev) < USB_DEV_STATE_ADDRESSED))

                    {
                        status = STATUS_INVALID_DEVICE_STATE;
                        goto ERROR_OUT1;
                    }

                    if (dev_state(pdev) == USB_DEV_STATE_ADDRESSED && !default_endp_handle(endp_handle))
                    {
                        status = STATUS_DEVICE_NOT_READY;
                        goto ERROR_OUT1;
                    }

                    if_idx = if_idx_from_handle(endp_handle);
                    endp_idx = endp_idx_from_handle(endp_handle);

                    //if_idx exceeds the upper limit
                    if (pdev->usb_config)
                    {
                        if (if_idx >= pdev->usb_config->if_count
                            || endp_idx >= pdev->usb_config->interf[if_idx].endp_count)
                        {
                            if (!default_endp_handle(endp_handle))
                            {
                                status = STATUS_INVALID_DEVICE_STATE;
                                goto ERROR_OUT1;
                            }
                        }
                    }

                    endp_from_handle(pdev, endp_handle, pendp);
                    // FIXME: don't know what evil will let loose
                    if (endp_type(pendp) != USB_ENDPOINT_XFER_CONTROL)
                    {
                        if (user_buffer_length > 0x100000)
                        {
                            status = STATUS_INVALID_PARAMETER;
                            goto ERROR_OUT1;
                        }
                    }

                    purb->pirp = irp;
                    purb->context = dev_mgr;
                    purb->reference = ctrl_code;

                    if (ctrl_code == IOCTL_SUBMIT_URB_RD || ctrl_code == IOCTL_SUBMIT_URB_WR)
                    {
                        if (ctrl_code == IOCTL_SUBMIT_URB_RD)
                            KeFlushIoBuffers(irp->MdlAddress, TRUE, TRUE);
                        else
                            KeFlushIoBuffers(irp->MdlAddress, FALSE, TRUE);

                        purb->data_buffer = user_buffer;
                        purb->data_length = user_buffer_length;
                        purb->completion = disp_urb_completion;
                    }
                    else
                    {
                        purb->completion = disp_noio_urb_completion;
                    }

                    unlock_dev(pdev, FALSE);

                    // we have to mark irp before the urb is scheduled to
                    // avoid race condition
                    IoMarkIrpPending(irp);
                    ASSERT(dev_mgr_register_irp(dev_mgr, irp, purb));
                    status = usb_submit_urb(dev_mgr, purb);
                    if (status != STATUS_PENDING)
                    {
                        IoGetCurrentIrpStackLocation((irp))->Control &= ~SL_PENDING_RETURNED;
                        dev_mgr_remove_irp(dev_mgr, irp);
                    }
                    usb_unlock_dev(pdev);
                    if (status != STATUS_PENDING)
                    {
                        irp->IoStatus.Status = status;
                        IoCompleteRequest(irp, IO_NO_INCREMENT);
                    }
                    return status;
                  ERROR_OUT1:
                    unlock_dev(pdev, FALSE);
                    usb_unlock_dev(pdev);
                    irp->IoStatus.Information = 0;
                    EXIT_DISPATCH(status, irp);
                }
            default:
                {
                    irp->IoStatus.Information = 0;
                    EXIT_DISPATCH(STATUS_NOT_IMPLEMENTED, irp);
                }
            }
        }
    default:
        {
            irp->IoStatus.Information = 0;
            break;
        }
    }
    EXIT_DISPATCH(STATUS_INVALID_DEVICE_REQUEST, irp);
}

/*#define IOCTL_GET_DEV_COUNT		CTL_CODE( FILE_HCD_DEV_TYPE, 4093, METHOD_BUFFERED, FILE_ANY_ACCESS )
//input_buffer and input_buffer_length is zero, output_buffer is to receive a dword value of the
//dev count, output_buffer_length must be no less than sizeof( long ).

#define IOCTL_ENUM_DEVICES 		CTL_CODE( FILE_HCD_DEV_TYPE, 4094, METHOD_BUFFERED, FILE_ANY_ACCESS )
//input_buffer is a dword value to indicate the count of elements in the array
//input_buffer_length is sizeof( long ), output_buffer is to receive a
//structure ENUM_DEV_ARRAY where dev_count is the elements hold in this array.

#define IOCTL_GET_DEV_DESC		CTL_CODE( FILE_HCD_DEV_TYPE, 4095, METHOD_BUFFERED, FILE_ANY_ACCESS )
//input_buffer is a structure GET_DEV_DESC_REQ, and the input_buffer_length is
//no less than sizeof( input_buffer ), output_buffer is a buffer to receive the
//requested dev's desc, and output_buffer_length specifies the length of the
//buffer

#define IOCTL_SUBMIT_URB_RD		CTL_CODE( FILE_HCD_DEV_TYPE, 4096, METHOD_IN_DIRECT, FILE_ANY_ACCESS )
#define IOCTL_SUBMIT_URB_WR 	CTL_CODE( FILE_HCD_DEV_TYPE, 4097, METHOD_OUT_DIRECT, FILE_ANY_ACCESS )
// input_buffer is a URB, and input_buffer_length is equal to or greater than
// sizeof( URB ); the output_buffer is a buffer to receive data from or send data
// to device. only the following urb fields can be accessed, others must be zeroed.
//  DEV_HANDLE 			endp_handle;
//	UCHAR             	setup_packet[8];   	//for control pipe
// the choosing of IOCTL_SUBMIT_URB_RD or IOCTL_SUBMIT_URB_WR should be determined
// by the current URB, for example, a request string from device will use XXX_RD,
// and a write to the bulk endpoint will use XXX_WR

#define IOCTL_SUBMIT_URB_NOIO	CTL_CODE( FILE_HCD_DEV_TYPE, 4098, METHOD_BUFFERED,	FILE_ANY_ACCESS )
// input_buffer is a URB, and input_buffer_length is equal to or greater than
// sizeof( URB ); the output_buffer is null and no output_buffer_length,
// only the following fields in urb can be accessed, others must be zeroed.
//  DEV_HANDLE 			endp_handle;
//	UCHAR             	setup_packet[8];   	//for control pipe
*/
