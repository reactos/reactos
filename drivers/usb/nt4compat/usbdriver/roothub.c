/**
 * roothub.c - USB driver stack project for Windows NT 4.0
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

//----------------------------------------------------------

BOOLEAN
rh_driver_destroy(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver)
{
    LONG i;
    PHCD hcd;

    if (dev_mgr == NULL)
        return FALSE;

    for(i = 0; i < dev_mgr->hcd_count; i++)
    {
        hcd = dev_mgr->hcd_array[i];
        // if( hcd->hcd_get_type( hcd ) != HCD_TYPE_UHCI )
        // continue;
        rh_destroy(hcd->hcd_get_root_hub(hcd));
    }
    return TRUE;
}

BOOLEAN
rh_driver_init(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver)
{

    PUSB_DEV rh;
    PUSB_CONFIGURATION_DESC pconfig_desc;
    PUSB_INTERFACE_DESC pif_desc;
    PUSB_ENDPOINT_DESC pendp_desc;
    PUSB_CONFIGURATION pconfig;
    PUSB_INTERFACE pif;
    PUSB_ENDPOINT pendp;
    PHUB2_EXTENSION phub_ext;
    NTSTATUS status;
    PHCD hcd;
    LONG i;

    if (dev_mgr == NULL || pdriver == NULL)
        return FALSE;

    //init driver structure, no PNP table functions
    pdriver->driver_desc.flags = USB_DRIVER_FLAG_DEV_CAPABLE;
    pdriver->driver_desc.vendor_id = 0xffff;    // USB Vendor ID
    pdriver->driver_desc.product_id = 0xffff;   // USB Product ID.
    pdriver->driver_desc.release_num = 0xffff;  // Release Number of Device

    pdriver->driver_desc.config_val = 0;        // Configuration Value
    pdriver->driver_desc.if_num = 0;    // Interface Number
    pdriver->driver_desc.if_class = USB_CLASS_HUB;      // Interface Class
    pdriver->driver_desc.if_sub_class = 0;      // Interface SubClass
    pdriver->driver_desc.if_protocol = 0;       // Interface Protocol

    pdriver->driver_desc.driver_name = "USB root hub";  // Driver name for Name Registry
    pdriver->driver_desc.dev_class = USB_CLASS_HUB;
    pdriver->driver_desc.dev_sub_class = 0;     // Device Subclass
    pdriver->driver_desc.dev_protocol = 0;      // Protocol Info.

    //pdriver->driver_init = rh_driver_init;                                        // initialized in dev_mgr_init_driver
    //pdriver->driver_destroy = rh_driver_destroy;
    pdriver->disp_tbl.version = 1;      // other fields of the dispatch table is not used since rh needs no pnp

    pdriver->driver_ext = 0;
    pdriver->driver_ext_size = 0;

    for(i = 0; i < dev_mgr->hcd_count; i++)
    {
        hcd = dev_mgr->hcd_array[i];
        //if( hcd->hcd_get_type( hcd ) != HCD_TYPE_UHCI )
        //    continue;

        if ((rh = dev_mgr_alloc_device(dev_mgr, hcd)) == NULL)
            return FALSE;

        rh->parent_dev = NULL;
        rh->port_idx = 0;
        rh->hcd = hcd;
        rh->flags = USB_DEV_CLASS_ROOT_HUB | USB_DEV_STATE_CONFIGURED;

        if (usb2(hcd))
            rh->flags |= USB_DEV_FLAG_HIGH_SPEED;

        rh->dev_driver = pdriver;

        rh->desc_buf_size = sizeof(USB_DEVICE_DESC)
            + sizeof(USB_CONFIGURATION_DESC)
            + sizeof(USB_INTERFACE_DESC)
            + sizeof(USB_ENDPOINT_DESC) + sizeof(USB_CONFIGURATION) + sizeof(HUB2_EXTENSION);

        rh->desc_buf = usb_alloc_mem(NonPagedPool, rh->desc_buf_size);

        if (rh->desc_buf == NULL)
        {
            return FALSE;
        }
        else
            RtlZeroMemory(rh->desc_buf, rh->desc_buf_size);

        rh->pusb_dev_desc = (PUSB_DEVICE_DESC) rh->desc_buf;

        rh->pusb_dev_desc->bLength = sizeof(USB_DEVICE_DESC);
        rh->pusb_dev_desc->bDescriptorType = USB_DT_DEVICE;
        rh->pusb_dev_desc->bcdUSB = 0x110;
        if (usb2(hcd))
            rh->pusb_dev_desc->bcdUSB = 0x200;
        rh->pusb_dev_desc->bDeviceClass = USB_CLASS_HUB;
        rh->pusb_dev_desc->bDeviceSubClass = 0;
        rh->pusb_dev_desc->bDeviceProtocol = 0;
        rh->pusb_dev_desc->bMaxPacketSize0 = 8;
        if (usb2(hcd))
        {
            rh->pusb_dev_desc->bDeviceProtocol = 1;
            rh->pusb_dev_desc->bMaxPacketSize0 = 64;
        }
        rh->pusb_dev_desc->idVendor = 0;
        rh->pusb_dev_desc->idProduct = 0;
        rh->pusb_dev_desc->bcdDevice = 0x100;
        rh->pusb_dev_desc->iManufacturer = 0;
        rh->pusb_dev_desc->iProduct = 0;
        rh->pusb_dev_desc->iSerialNumber = 0;
        rh->pusb_dev_desc->bNumConfigurations = 1;

        pconfig_desc = (PUSB_CONFIGURATION_DESC) & rh->desc_buf[sizeof(USB_DEVICE_DESC)];
        pif_desc = (PUSB_INTERFACE_DESC) & pconfig_desc[1];
        pendp_desc = (PUSB_ENDPOINT_DESC) & pif_desc[1];

        pconfig_desc->bLength = sizeof(USB_CONFIGURATION_DESC);
        pconfig_desc->bDescriptorType = USB_DT_CONFIG;

        pconfig_desc->wTotalLength = sizeof(USB_CONFIGURATION_DESC)
            + sizeof(USB_INTERFACE_DESC) + sizeof(USB_ENDPOINT_DESC);

        pconfig_desc->bNumInterfaces = 1;
        pconfig_desc->bConfigurationValue = 1;
        pconfig_desc->iConfiguration = 0;
        pconfig_desc->bmAttributes = 0Xe0;      //self-powered and support remoke wakeup
        pconfig_desc->MaxPower = 0;

        pif_desc->bLength = sizeof(USB_INTERFACE_DESC);
        pif_desc->bDescriptorType = USB_DT_INTERFACE;
        pif_desc->bInterfaceNumber = 0;
        pif_desc->bAlternateSetting = 0;
        pif_desc->bNumEndpoints = 1;
        pif_desc->bInterfaceClass = USB_CLASS_HUB;
        pif_desc->bInterfaceSubClass = 0;
        pif_desc->bInterfaceProtocol = 0;
        pif_desc->iInterface = 0;

        pendp_desc->bLength = sizeof(USB_ENDPOINT_DESC);
        pendp_desc->bDescriptorType = USB_DT_ENDPOINT;
        pendp_desc->bEndpointAddress = 0x81;
        pendp_desc->bmAttributes = 0x03;
        pendp_desc->wMaxPacketSize = 8;
        pendp_desc->bInterval = USB_HUB_INTERVAL;
        if (usb2(hcd))
            pendp_desc->bInterval = 0x0c;

        pconfig = rh->usb_config = (PUSB_CONFIGURATION) & pendp_desc[1];
        rh->active_config_idx = 0;
        pconfig->pusb_config_desc = pconfig_desc;
        pconfig->if_count = 1;
        pconfig->pusb_dev = rh;
        pif = &pconfig->interf[0];

        pif->endp_count = 1;
        pendp = &pif->endp[0];
        pif->pusb_config = pconfig;
        pif->pusb_if_desc = pif_desc;

        pif->if_ext_size = 0;
        pif->if_ext = NULL;

        phub_ext = (PHUB2_EXTENSION) & pconfig[1];
        phub_ext->port_count = 2;

        if (usb2(hcd))
        {
            // port count is configurable in usb2
            hcd->hcd_dispatch(hcd, HCD_DISP_READ_PORT_COUNT, &phub_ext->port_count);
        }

        {
            int j;
            for(j = 0; j < phub_ext->port_count; j++)
            {
                psq_init(&phub_ext->port_status_queue[j]);
                phub_ext->child_dev[j] = NULL;
                usb_dbg_print(DBGLVL_MAXIMUM, ("rh_driver_init(): port[ %d ].flag=0x%x\n",
                                               j, phub_ext->port_status_queue[j].port_flags));
            }
        }

        phub_ext->pif = pif;
        phub_ext->hub_desc.bLength = sizeof(USB_HUB_DESCRIPTOR);
        phub_ext->hub_desc.bDescriptorType = USB_DT_HUB;
        phub_ext->hub_desc.bNbrPorts = (UCHAR) phub_ext->port_count;
        phub_ext->hub_desc.wHubCharacteristics = 0;
        phub_ext->hub_desc.bPwrOn2PwrGood = 0;
        phub_ext->hub_desc.bHubContrCurrent = 50;

        rh->dev_ext = (PBYTE) phub_ext;
        rh->dev_ext_size = sizeof(HUB2_EXTENSION);

        rh->default_endp.flags = USB_ENDP_FLAG_DEFAULT_ENDP;
        InitializeListHead(&rh->default_endp.urb_list);
        rh->default_endp.pusb_if = (PUSB_INTERFACE) rh;
        rh->default_endp.pusb_endp_desc = NULL; //???
        rh->time_out_count = 0;
        rh->error_count = 0;

        InitializeListHead(&pendp->urb_list);
        pendp->flags = 0;
        pendp->pusb_endp_desc = pendp_desc;
        pendp->pusb_if = pif;

        //add to device list
        InsertTailList(&dev_mgr->dev_list, &rh->dev_link);
        hcd->hcd_set_root_hub(hcd, rh);
        status = hub_start_int_request(rh);
        pdriver->driver_ext = 0;
    }
    return TRUE;
}

//to be the reverse of what init does, we assume that the timer is now killed
//int is disconnected and the hub thread will not process event anymore
BOOLEAN
rh_destroy(PUSB_DEV pdev)
{
    PUSB_DEV rh;
    PUSB_DEV_MANAGER dev_mgr;

    if (pdev == NULL)
        return FALSE;

    dev_mgr = dev_mgr_from_dev(pdev);

    //???
    rh = pdev->hcd->hcd_get_root_hub(pdev->hcd);
    if (rh == pdev)
    {
        //free all the buf
        dev_mgr_free_device(dev_mgr, rh);
        //dev_mgr->root_hub = NULL;
    }

    return TRUE;
}

VOID
rh_timer_svc_int_completion(PUSB_DEV pdev, PVOID context)
{
    PURB purb;
    PHCD hcd;
    USE_BASIC_NON_PENDING_IRQL;

    if (pdev == NULL || context == NULL)
        return;

    purb = (PURB) context;

    lock_dev(pdev, TRUE);

    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        pdev->ref_count -= 2;   //      one for timer_svc and one for urb, for those rh requests
        unlock_dev(pdev, TRUE);
        usb_free_mem(purb);
        usb_dbg_print(DBGLVL_MAXIMUM, ("rh_timer_svc_int_completion(): the dev is zomb, 0x%x\n", pdev));
        return;
    }

    hcd = pdev->hcd;
    if (purb->data_length < 1)
    {
        purb->status = STATUS_INVALID_PARAMETER;
        unlock_dev(pdev, TRUE);
        goto LBL_OUT;
    }

    pdev->hcd->hcd_dispatch(pdev->hcd, HCD_DISP_READ_RH_DEV_CHANGE, purb->data_buffer);
    purb->status = STATUS_SUCCESS;
    unlock_dev(pdev, TRUE);

  LBL_OUT:
    hcd->hcd_generic_urb_completion(purb, purb->context);

    lock_dev(pdev, TRUE);
    pdev->ref_count -= 2;
    //      one for timer_svc and one for urb, for those rh requests
    //      that completed immediately, the ref_count of the dev for
    //      that urb won't increment and for normal hub request
    //      completion, hcd_generic_urb_completion will be called
    //  by the xhci_dpc_callback, and the ref_count for the urb
    //  is maintained there. So only rh's timer-svc cares refcount
    //  when hcd_generic_urb_completion is called.
    usb_dbg_print(DBGLVL_ULTRA, ("rh_timer_svc_int_completion(): rh's ref_count=0x%x\n", pdev->ref_count));
    unlock_dev(pdev, TRUE);
    usb_dbg_print(DBGLVL_ULTRA, ("rh_timer_svc_int_completion(): exitiing...\n"));
    return;
}

VOID
rh_timer_svc_reset_port_completion(PUSB_DEV pdev, PVOID context)
{
    PURB purb;
    ULONG i;
    PHUB2_EXTENSION hub_ext;
    PLIST_ENTRY pthis, pnext;
    PUSB_DEV_MANAGER dev_mgr;
    PUSB_CTRL_SETUP_PACKET psetup;

    USE_BASIC_NON_PENDING_IRQL;

    if (pdev == NULL || context == NULL)
        return;

    dev_mgr = dev_mgr_from_dev(pdev); //readonly and hold ref_count

    //block the rh polling
    KeAcquireSpinLockAtDpcLevel(&dev_mgr->timer_svc_list_lock);
    if (IsListEmpty(&dev_mgr->timer_svc_list) == FALSE)
    {
        ListFirst(&dev_mgr->timer_svc_list, pthis);
        while (pthis)
        {
            if (((PTIMER_SVC) pthis)->pdev == pdev && ((PTIMER_SVC) pthis)->threshold == RH_INTERVAL)
            {
                ((PTIMER_SVC) pthis)->threshold = RH_INTERVAL + 0x800000;
                break;
            }

            ListNext(&dev_mgr->timer_svc_list, pthis, pnext);
            pthis = pnext;
        }
    }
    KeReleaseSpinLockFromDpcLevel(&dev_mgr->timer_svc_list_lock);

    purb = (PURB) context;
    psetup = (PUSB_CTRL_SETUP_PACKET) purb->setup_packet;

    lock_dev(pdev, TRUE);
    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        //purb->status = STATUS_ERROR;
        //pdev->hcd->hcd_generic_urb_completion( purb, purb->context );

        pdev->ref_count -= 2;
        unlock_dev(pdev, TRUE);
        usb_free_mem(purb);
        return;
    }

    i = pdev->hcd->hcd_rh_reset_port(pdev->hcd, (UCHAR) psetup->wIndex);

    hub_ext = hub_ext_from_dev(pdev);

    {
        USHORT temp;
        PUCHAR pbuf;
        if (psetup->wIndex < 16)
        {
            temp = 1 << psetup->wIndex;
            pbuf = (PUCHAR) & temp;
            if (temp > 128)
                pbuf++;
            hub_ext->int_data_buf[psetup->wIndex / 8] |= *pbuf;
            if (i == TRUE)
                hub_ext->rh_port_status[psetup->wIndex].wPortChange |= USB_PORT_STAT_C_RESET;
            else                // notify that is not a high speed device, will lost definitely
                hub_ext->rh_port_status[psetup->wIndex].wPortChange |= USB_PORT_STAT_C_CONNECTION;
        }
    }

    //???how to construct port status map
    // decrease the timer_svc ref-count
    pdev->ref_count--;
    unlock_dev(pdev, TRUE);

    purb->status = STATUS_SUCCESS;
    //we delegate the completion to the rh_timer_svc_int_completion.
    //this function is equivalent to hub_start_reset_port_completion

    usb_free_mem(purb);

    //expire the rh polling timer
    KeAcquireSpinLockAtDpcLevel(&dev_mgr->timer_svc_list_lock);
    if (IsListEmpty(&dev_mgr->timer_svc_list) == FALSE)
    {
        ListFirst(&dev_mgr->timer_svc_list, pthis);
        while (pthis)
        {
            if (((PTIMER_SVC) pthis)->pdev == pdev &&
                ((PTIMER_SVC) pthis)->threshold == RH_INTERVAL + 0x800000)
            {
                ((PTIMER_SVC) pthis)->counter = RH_INTERVAL;
                ((PTIMER_SVC) pthis)->threshold = RH_INTERVAL;
                break;
            }

            ListNext(&dev_mgr->timer_svc_list, pthis, pnext);
            pthis = pnext;
        }
    }
    KeReleaseSpinLockFromDpcLevel(&dev_mgr->timer_svc_list_lock);

    lock_dev(pdev, TRUE);
    pdev->ref_count--;
    unlock_dev(pdev, TRUE);
    return;
}
