/**
 * compdrv.c - USB driver stack project for Windows NT 4.0
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

//this driver is part of the dev manager responsible to manage if device
#include "usbdriver.h"

VOID compdev_set_cfg_completion(PURB purb, PVOID context);
VOID compdev_select_driver(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle);
BOOLEAN compdev_connect(PDEV_CONNECT_DATA param, DEV_HANDLE dev_handle);
BOOLEAN compdev_stop(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle);
BOOLEAN compdev_disconnect(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle);

BOOLEAN
compdev_driver_init(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver)
{
    if (dev_mgr == NULL || pdriver == NULL)
        return FALSE;

    pdriver->driver_desc.flags = USB_DRIVER_FLAG_DEV_CAPABLE;
    pdriver->driver_desc.vendor_id = 0xffff;    // USB Vendor ID
    pdriver->driver_desc.product_id = 0xffff;   // USB Product ID.
    pdriver->driver_desc.release_num = 0x100;   // Release Number of Device

    pdriver->driver_desc.config_val = 0;        // Configuration Value
    pdriver->driver_desc.if_num = 0;            // Interface Number
    pdriver->driver_desc.if_class = 0;          // Interface Class
    pdriver->driver_desc.if_sub_class = 0;      // Interface SubClass
    pdriver->driver_desc.if_protocol = 0;       // Interface Protocol

    pdriver->driver_desc.driver_name = "USB composit dev driver";       // Driver name for Name Registry
    pdriver->driver_desc.dev_class = USB_CLASS_PER_INTERFACE;
    pdriver->driver_desc.dev_sub_class = 0;     // Device Subclass
    pdriver->driver_desc.dev_protocol = 0;      // Protocol Info.

    //we have no extra data sturcture currently
    pdriver->driver_ext = NULL;
    pdriver->driver_ext_size = 0;

    pdriver->disp_tbl.version = 1;
    pdriver->disp_tbl.dev_connect = compdev_connect;
    pdriver->disp_tbl.dev_disconnect = compdev_disconnect;
    pdriver->disp_tbl.dev_stop = compdev_stop;
    pdriver->disp_tbl.dev_reserved = NULL;

    return TRUE;
}

BOOLEAN
compdev_driver_destroy(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver)
{
    UNREFERENCED_PARAMETER(dev_mgr);
    UNREFERENCED_PARAMETER(pdriver);
    return TRUE;
}

BOOLEAN
compdev_connect(PDEV_CONNECT_DATA param, DEV_HANDLE dev_handle)
{
    PURB purb;
    PUSB_CTRL_SETUP_PACKET psetup;
    NTSTATUS status;
    PUCHAR buf;
    LONG credit, i, j;
    PUSB_CONFIGURATION_DESC pconfig_desc;
    PUSB_INTERFACE_DESC pif_desc;
    PUSB_DEV_MANAGER dev_mgr;

    if (param == NULL || dev_handle == 0)
        return FALSE;

    dev_mgr = param->dev_mgr;

    // let's set the configuration
    purb = usb_alloc_mem(NonPagedPool, sizeof(URB));
    if (purb == NULL)
        return FALSE;

    buf = usb_alloc_mem(NonPagedPool, 512);
    if (buf == NULL)
    {
        usb_dbg_print(DBGLVL_MAXIMUM, ("compdev_connect(): can not alloc buf\n"));
        usb_free_mem(purb);
        return FALSE;
    }

    // before we set the configuration, let's search to find if there
    // exist interfaces we supported
    psetup = (PUSB_CTRL_SETUP_PACKET) (purb)->setup_packet;
    urb_init((purb));
    purb->endp_handle = dev_handle | 0xffff;
    purb->data_buffer = buf;
    purb->data_length = 512;
    purb->completion = NULL;    // this is an immediate request, no completion required
    purb->context = NULL;
    purb->reference = 0;
    psetup->bmRequestType = 0x80;
    psetup->bRequest = USB_REQ_GET_DESCRIPTOR;
    psetup->wValue = USB_DT_CONFIG << 8;
    psetup->wIndex = 0;
    psetup->wLength = 512;

    status = usb_submit_urb(dev_mgr, purb);
    if (status == STATUS_PENDING)
    {
        TRAP();
        usb_free_mem(buf);
        usb_free_mem(purb);
        return FALSE;
    }

    // let's scan the interfacs for those we recognize
    pconfig_desc = (PUSB_CONFIGURATION_DESC) buf;
    if (pconfig_desc->wTotalLength > 512)
    {
        usb_free_mem(buf);
        usb_free_mem(purb);
        usb_dbg_print(DBGLVL_MAXIMUM, ("compdev_connect(): error, bad configuration desc\n"));
        return FALSE;
    }

    pif_desc = (PUSB_INTERFACE_DESC) & pconfig_desc[1];
    for(i = 0, credit = 0; i < (LONG) pconfig_desc->bNumInterfaces; i++)
    {
        for(j = 0; j < DEVMGR_MAX_DRIVERS; j++)
        {
            credit = dev_mgr_score_driver_for_if(dev_mgr, &dev_mgr->driver_list[j], pif_desc);
            if (credit)
                break;
        }
        if (credit)
            break;

        if (usb_skip_if_and_altif((PUCHAR *) & pif_desc))
            break;
    }

    i = pconfig_desc->bConfigurationValue;
    usb_free_mem(buf);
    buf = NULL;
    if (credit == 0)
    {
        usb_free_mem(purb);
        usb_dbg_print(DBGLVL_MAXIMUM, ("compdev_connect(): oops..., no supported interface found\n"));
        return FALSE;
    }

    //set the configuration
    urb_init(purb);
    purb->endp_handle = dev_handle | 0xffff;
    purb->data_buffer = NULL;
    purb->data_length = 0;
    purb->completion = compdev_set_cfg_completion;
    purb->context = dev_mgr;
    purb->reference = (ULONG) param->pdriver;
    psetup->bmRequestType = 0;
    psetup->bRequest = USB_REQ_SET_CONFIGURATION;
    psetup->wValue = (USHORT) i;
    psetup->wIndex = 0;
    psetup->wLength = 0;

    usb_dbg_print(DBGLVL_MAXIMUM, ("compdev_connect(): start config the device, cfgval=%d\n", i));
    status = usb_submit_urb(dev_mgr, purb);

    if (status != STATUS_PENDING)
    {
        usb_free_mem(purb);

        if (status == STATUS_SUCCESS)
            return TRUE;

        return FALSE;
    }

    return TRUE;
}

VOID
compdev_event_select_if_driver(PUSB_DEV pdev, ULONG event, ULONG context, ULONG param)
{
    PUSB_DEV_MANAGER dev_mgr;
    DEV_HANDLE dev_handle;

    UNREFERENCED_PARAMETER(param);
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(event);

    if (pdev == NULL)
        return;

    //
    // RtlZeroMemory( &cd, sizeof( cd ) );
    //
    dev_mgr = dev_mgr_from_dev(pdev);
    dev_handle = usb_make_handle(pdev->dev_id, 0, 0);
    compdev_select_driver(dev_mgr, dev_handle);
    return;
}

BOOLEAN
compdev_post_event_select_driver(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle)
{
    PUSB_EVENT pevent;
    BOOLEAN bret;
    PUSB_DEV pdev;
    USE_BASIC_NON_PENDING_IRQL;

    if (dev_mgr == NULL || dev_handle == 0)
        return FALSE;

    if (usb_query_and_lock_dev(dev_mgr, dev_handle, &pdev) != STATUS_SUCCESS)
        return FALSE;

    KeAcquireSpinLockAtDpcLevel(&dev_mgr->event_list_lock);
    lock_dev(pdev, TRUE);

    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        bret = FALSE;
        goto LBL_OUT;
    }

    pevent = alloc_event(&dev_mgr->event_pool, 1);
    if (pevent == NULL)
    {
        bret = FALSE;
        goto LBL_OUT;
    }
    pevent->flags = USB_EVENT_FLAG_ACTIVE;
    pevent->event = USB_EVENT_DEFAULT;
    pevent->pdev = pdev;
    pevent->context = 0;
    pevent->param = 0;
    pevent->pnext = 0;          //vertical queue for serialized operation
    pevent->process_event = compdev_event_select_if_driver;
    pevent->process_queue = event_list_default_process_queue;

    InsertTailList(&dev_mgr->event_list, &pevent->event_link);
    KeSetEvent(&dev_mgr->wake_up_event, 0, FALSE);      // wake up the dev_mgr_thread
    bret = TRUE;

  LBL_OUT:

    unlock_dev(pdev, TRUE);
    KeReleaseSpinLockFromDpcLevel(&dev_mgr->event_list_lock);
    usb_unlock_dev(pdev);
    return bret;
}

VOID
compdev_set_cfg_completion(PURB purb, PVOID context)
{
    DEV_HANDLE dev_handle;
    PUSB_DEV_MANAGER dev_mgr;
    PUSB_DRIVER pdriver;
    NTSTATUS status;
    PUSB_DEV pdev;
    USE_BASIC_NON_PENDING_IRQL;

    if (purb == NULL || context == NULL)
        return;

    dev_handle = purb->endp_handle & ~0xffff;
    dev_mgr = (PUSB_DEV_MANAGER) context;
    pdriver = (PUSB_DRIVER) purb->reference;

    if (purb->status != STATUS_SUCCESS)
    {
        usb_free_mem(purb);
        return;
    }

    usb_free_mem(purb);
    purb = NULL;

    // set the dev state
    status = usb_query_and_lock_dev(dev_mgr, dev_handle, &pdev);
    if (status != STATUS_SUCCESS)
    {
        usb_unlock_dev(pdev);
        return;
    }
    // safe to release the pdev ref since we are in urb completion
    usb_unlock_dev(pdev);

    lock_dev(pdev, TRUE);
    if (dev_state(pdev) >= USB_DEV_STATE_BEFORE_ZOMB)
    {
        unlock_dev(pdev, TRUE);
        return;
    }

    if (dev_mgr_set_driver(dev_mgr, dev_handle, pdriver, pdev) == FALSE)
        return;

    //transit the state to configured
    pdev->flags &= ~USB_DEV_STATE_MASK;
    pdev->flags |= USB_DEV_STATE_CONFIGURED;
    unlock_dev(pdev, TRUE);

    //
    // we change to use our thread for driver choosing. it will reduce
    // the race condition when different pnp event comes simultaneously
    //
    usb_dbg_print(DBGLVL_MAXIMUM, ("compdev_set_cfg_completion(): start select driver for the dev\n"));
    compdev_post_event_select_driver(dev_mgr, dev_handle);

    return;
}

VOID
compdev_select_driver(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle)
{
    URB urb;
    LONG i, j, k, credit;
    ULONG dev_id;
    PUCHAR buf;
    NTSTATUS status;
    PUSB_DRIVER pcand, ptemp_drv;

    PUSB_CTRL_SETUP_PACKET psetup;
    PUSB_INTERFACE_DESC pif_desc;
    PUSB_CONFIGURATION_DESC pconfig_desc;
    PUSB_DEV pdev;

    usb_dbg_print(DBGLVL_MAXIMUM, ("compdev_select_driver(): entering...\n"));

    dev_id = dev_handle >> 16;

    buf = usb_alloc_mem(NonPagedPool, 512);

    if (buf == NULL)
        return;

    // now let's get the descs, one configuration
    urb_init(&urb);
    psetup = (PUSB_CTRL_SETUP_PACKET) urb.setup_packet;
    urb.endp_handle = dev_handle | 0xffff;
    urb.data_buffer = buf;
    urb.data_length = 512;
    urb.completion = NULL;      // this is an immediate request, no completion required
    urb.context = NULL;
    urb.reference = 0;
    psetup->bmRequestType = 0x80;
    psetup->bRequest = USB_REQ_GET_DESCRIPTOR;
    psetup->wValue = USB_DT_CONFIG << 8;
    psetup->wIndex = 0;
    psetup->wLength = 512;

    status = usb_submit_urb(dev_mgr, &urb);
    if (status == STATUS_PENDING)
    {
        TRAP();
    }

    // let's scan the interfaces for those we recognize
    pconfig_desc = (PUSB_CONFIGURATION_DESC) buf;
    if (pconfig_desc->wTotalLength > 512)
    {
        usb_free_mem(buf);
        usb_dbg_print(DBGLVL_MAXIMUM, ("compdev_select_driver(): error, bad configuration desc\n"));
        return;
    }
    pif_desc = (PUSB_INTERFACE_DESC) & pconfig_desc[1];

    if (usb_query_and_lock_dev(dev_mgr, dev_handle, &pdev) != STATUS_SUCCESS)
    {
        usb_free_mem(buf);
        usb_dbg_print(DBGLVL_MAXIMUM, ("compdev_select_driver(): error, dev does not exist\n"));
        return;
    }

    usb_dbg_print(DBGLVL_MAXIMUM, ("compdev_select_driver(): got %d interfaces\n",
        (LONG)pconfig_desc->bNumInterfaces));

    for(i = 0; i < (LONG) pconfig_desc->bNumInterfaces; i++)
    {
        for(j = 0, credit = 0, pcand = NULL; j < DEVMGR_MAX_DRIVERS; j++)
        {
            ptemp_drv = &dev_mgr->driver_list[j];
            k = dev_mgr_score_driver_for_if(dev_mgr, ptemp_drv, pif_desc);
            if (k > credit)
                credit = k, pcand = ptemp_drv;
        }

        if (credit)
        {
            // ok, we find one
            DEV_CONNECT_DATA param;

            if (pcand->disp_tbl.dev_connect)
            {
                param.dev_mgr = dev_mgr;
                param.pdriver = pcand;
                param.dev_handle = 0;
                param.if_desc = pif_desc;
                pcand->disp_tbl.dev_connect(&param, usb_make_handle(dev_id, i, 0));
            }
        }
        if (usb_skip_if_and_altif((PUCHAR *) & pif_desc) == FALSE)
        {
            break;
        }
    }
    usb_unlock_dev(pdev);

    if (buf)
    {
        usb_free_mem(buf);
        buf = NULL;
    }
    return;
}

BOOLEAN
compdev_stop(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle)
{
    PUSB_DEV pdev;
    LONG i;
    ULONG dev_id;
    PUSB_DRIVER pdrv;
    NTSTATUS status;

    if (dev_mgr == NULL || dev_handle == 0)
        return FALSE;

    pdev = NULL;
    dev_id = dev_handle >> 16;
    status = usb_query_and_lock_dev(dev_mgr, dev_handle, &pdev);
    if (pdev)
    {
        if (pdev->usb_config)
        {
            for(i = 0; i < pdev->usb_config->if_count; i++)
            {
                if ((pdrv = pdev->usb_config->interf[i].pif_drv))
                {
                    pdrv->disp_tbl.dev_stop(dev_mgr, usb_make_handle(dev_id, i, 0));
                }
            }
        }
    }
    if (status == STATUS_SUCCESS)
    {
        usb_unlock_dev(pdev);
    }
    return TRUE;
}

BOOLEAN
compdev_disconnect(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle)
{
    PUSB_DEV pdev;
    LONG i;
    ULONG dev_id;
    PUSB_DRIVER pdrv;
    NTSTATUS status;

    if (dev_mgr == NULL || dev_handle == 0)
        return FALSE;

    pdev = NULL;
    dev_id = dev_handle >> 16;
    status = usb_query_and_lock_dev(dev_mgr, dev_handle, &pdev);
    if (pdev)
    {
        if (pdev->usb_config)
        {
            for(i = 0; i < pdev->usb_config->if_count; i++)
            {
                if ((pdrv = pdev->usb_config->interf[i].pif_drv))
                {
                    pdrv->disp_tbl.dev_disconnect(dev_mgr, usb_make_handle(dev_id, i, 0));
                }
            }
        }
    }
    if (status == STATUS_SUCCESS)
    {
        usb_unlock_dev(pdev);
    }
    return TRUE;
}

// note:
// dev_mgr_set_driver seems to be dangeous since compdev, gendrv and hub and
// umss use it to set the driver while there may exist race condition when the
// dev_mgr_disconnect_dev is called. If the driver is set and the
// disconnect_dev cut in immediately, the stop or disconnect may not function
// well. Now hub and compdev's set dev_mgr_set_driver  are ok.
//
// another danger comes from umss's dev irp processing. This may confuse the device
// when the disk is being read or written, and at the same time dmgrdisp's dispatch
// route irp request to the device a control request.
